#ifndef INFRA_KAFKA_PRODUCER_H_
#define INFRA_KAFKA_PRODUCER_H_

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "glog/logging.h"
#include "infra/kafka/EventCallback.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

// A wrapper class for Rdkafka::Producer
// TODO(yunjing): expose a sync API to produce one message at a time
class Producer : public RdKafka::PartitionerCb, public RdKafka::DeliveryReportCb, public EventCallback {
 public:
  using DeliveryHandler = void (*)(const RdKafka::Message&);
  using Partitioner = int (*)(const RdKafka::Topic&, const std::string&, int, void*);
  struct Config {
    // Report error when delivery failed. Clients may acess msg_opaque to improve delivery handling
    DeliveryHandler deliveryHandler = [](const RdKafka::Message& message) {
      if (message.err() != RdKafka::ERR_NO_ERROR) {
        LOG(ERROR) << "Kafka message produce for partition " << message.partition() << " of " << message.topic_name()
                   << " failed: " << message.errstr();
      }
    };
    // Pre-configured partition, which defaults to unassigned. This instructs the producer to send to a fixed
    // partition explicitly. However, client can still overwrite it at sending time.
    int partition = RdKafka::Topic::PARTITION_UA;
    // No partitioner defined by default. This is only required when clients pass in a key without partition assigned
    Partitioner partitioner = nullptr;
    // Compress kafka messages by default
    bool useCompression = true;
    // Prioritize throughput by default
    bool lowLatency = false;
    // Additional librdkafka topic level configs to set directly
    std::unordered_map<std::string, std::string> topicConfigs;
  };

  Producer(const std::string& brokerList, const std::string& topicStr, Config config)
      : brokerList_(brokerList),
        topicStr_(topicStr),
        partition_(config.partition),
        partitioner_(config.partitioner),
        useCompression_(config.useCompression),
        lowLatency_(config.lowLatency),
        topicConfigs_(std::move(config.topicConfigs)),
        deliveryHandler_(config.deliveryHandler),
        conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
        topicConf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)) {
    initialize();
  }

  virtual ~Producer() {}

  void destroy() {
    waitForAck();
    LOG(INFO) << "Kafka producer send queue is clean";
  }

  // Implement dr_cb required by RdKafka::DeliveryReportCb.
  // Instead of overriding this, clients should pass in a DeliveryHandler in configuration.
  void dr_cb(RdKafka::Message& message) override {
    deliveryHandler_(message);
  }

  // Implement partitioner_cb required by RdKafka::PartitionerCb.
  // Clients may override partitioning behavior by passing in a Partitioner in configuration.
  int partitioner_cb(const RdKafka::Topic* topic, const std::string* key, int partitions, void* msgOpaque) override {
    return partitioner_(*topic, *key, partitions, msgOpaque);
  }

  // TODO(yunjing): find a better way to inject this. We could inject it via the config object, but our current
  // implementation of bootstrap process does not take a handler function pointer.
  void setDeliveryHandler(DeliveryHandler handler) {
    deliveryHandler_ = handler;
  }

  // produceAsync is a more friendly and more restricted async producer API
  // There a few restrictions/simplifications compared to the original rdkafka producer API:
  // 1. `msgflags` is set to RdKafka::Producer::RK_MSG_COPY to simplify memory management. RK_MSG_FREE might be more
  //    efficient, but it requires the caller to delete the message when produce failed.
  // 2. `payload` pointer is defined `const` so that client code can avoid doing `const_cast` explicitly.
  // 3. Both `key` and `msgOpaque` are set `nullptr` by default as client code rarely uses them. If `key` is not null,
  //    then `partitioner` must be specified.
  //
  // NOTE this this method is named `produceAsync` to emphasize the fact that librdkafka's producer API is
  // asynchronous by design. Compared to a synchronous API, its advantage is improved throughput because messages
  // are buffered and sent to brokers in larger batches. On the flip side, such buffering may cause message lose
  // when the process crashed before buffered messages are acknowledged by brokers.
  RdKafka::ErrorCode produceAsync(const void* payload, size_t len, int partition, const std::string* key = nullptr,
                                  void* msgOpaque = nullptr) {
    return producer_->produce(topic_.get(), partition, RdKafka::Producer::RK_MSG_COPY, const_cast<void *>(payload),
                              len, key, msgOpaque);
  }

  // An overloaded produceAsync function that uses the pre-configured partition
  RdKafka::ErrorCode produceAsync(const void* payload, size_t len, const std::string* key = nullptr,
                                  void* msgOpaque = nullptr) {
    return produceAsync(payload, len, partition_, key, msgOpaque);
  }

  // A convenience function that retries when producer buffer is full and calls LOG(FATAL) when producerAsync returns
  // any other error. Retry happens once every second
  void produceAsyncFatalOnError(const std::string& msg, int partition) {
    RdKafka::ErrorCode errorCode;
    while ((errorCode = produceAsync(msg.data(), msg.size(), partition)) != RdKafka::ERR_NO_ERROR) {
      if (errorCode == RdKafka::ERR__QUEUE_FULL) {
        LOG(WARNING) << "Producing kafka messages too fast. Throttling by sleeping for 1 second";
        std::this_thread::sleep_for(std::chrono::seconds(1));
      } else {
        LOG(FATAL) << "Error producing kafka message: " << RdKafka::err2str(errorCode);
      }
    }
  }

  // An overloaded produceAsyncFatalOnError function that uses the pre-configured partition
  void produceAsyncFatalOnError(const std::string& msg) {
    produceAsyncFatalOnError(msg, partition_);
  }

  // Wait for the all the outstanding messages and sent/ack'd by brokers.
  // NOTE that using produceAsync + waitForAck is not equivalent to a sync API since the library's send queue
  // introduces delays to batch messages before sending, which implies additional delays when the intention is to send
  // individual messages synchronously. This problem can be solved by setting the delay to the minimum 1ms, which is in
  // the TODO list.
  // The benefit of produceAsync + waitForAck is to allow batching while effectively check pointing the delivery of
  // each batch. Clients may check every N messages or wait after all messages have been produced.
  void waitForAck() {
    producer_->poll(0);
    while (producer_->outq_len() > 0) {
      producer_->poll(1000);
    }
  }

  // Give callbacks (e.g., event and delivery) a chance to run. This should be called at regular intervals
  void pollCallbacks(int timeout = 0) {
    producer_->poll(timeout);
  }

  bool isPartitionAssigned() const {
    return partition_ >= 0 && partition_ != RdKafka::Topic::PARTITION_UA;
  }

 private:
  void initialize();

  void setConf(const std::string& name, const std::string& value) {
    std::string errstr;
    if (conf_->set(name, value, errstr) != RdKafka::Conf::CONF_OK) {
      LOG(FATAL) << "Setting Kafka producer configuration failed: " << errstr;
    }
  }

  const std::string brokerList_;
  const std::string topicStr_;
  const int partition_;
  const Partitioner partitioner_;
  const bool useCompression_;
  const bool lowLatency_;
  const std::unordered_map<std::string, std::string> topicConfigs_;
  DeliveryHandler deliveryHandler_;
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> topicConf_;
  std::unique_ptr<RdKafka::Producer> producer_;
  std::unique_ptr<RdKafka::Topic> topic_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_PRODUCER_H_
