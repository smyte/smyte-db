#ifndef INFRA_KAFKA_CONSUMER_H_
#define INFRA_KAFKA_CONSUMER_H_

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "glog/logging.h"
#include "infra/kafka/AbstractConsumer.h"
#include "infra/kafka/EventCallback.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

class Consumer : public AbstractConsumer, public EventCallback {
 public:
  Consumer(const std::string& brokerList, const std::string& topicStr, int partition, const std::string& groupId)
      : brokerList_(brokerList),
        topicStr_(topicStr),
        partition_(partition),
        groupId_(groupId),
        conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)) {}

  virtual ~Consumer() {}

  // Initialize the underlying kafka consumer and verify topic, partition, and initial offset are all valid
  void init(int64_t initialOffset) override;

  void destroy(void) override {
    LOG(INFO) << "Stopping kafka consumer";
    // stop the infinite loop in the consumer thread
    stop();

    waitForStop();

    // release librdkafka resources
    if (consumer_) {
      // stop the consumer itself
      consumer_->close();
      consumer_.reset(nullptr);
      // allow graceful shutdown
      RdKafka::wait_destroyed(3000);
    }
    LOG(INFO) << "Kafka consumer destroyed";
  }

  // More fine-grained methods to control the life-cycle of message consumption
  // Default implementation is available for everything but processOneMessage

  // Load committed offset from persistent storage.
  // The default implementation is always start from the beginning
  int64_t loadCommittedKafkaOffset(void) override {
    return RdKafka::Topic::OFFSET_BEGINNING;
  }

  // Process a batch of messages.
  //
  // Subclasses may override it to pass in an opaque object for processing each batch of messages
  // one useful opaque object would be rocksdb::WriteBatch, which allows individual messages to add
  // key/value pairs to the batch but only write to database at the end of batch processing.
  //
  // Overriding method must call consumeBatch.
  void processBatch(int timeoutMs) override {
    consumeBatch(timeoutMs, nullptr);
  }

  // Process one message.
  virtual void processOne(const RdKafka::Message& msg, void* opaque) = 0;

  // Process one message with an error
  virtual void processError(const RdKafka::Message& msgWithError, void* opaque) {
    switch (msgWithError.err()) {
    // read timeout or partition EOF are not really errors
    case RdKafka::ERR__TIMED_OUT:
      break;
    case RdKafka::ERR__PARTITION_EOF:
      LOG(INFO) << "No more messages for partition " << partition_ << " of " << topicStr_;
      break;
    default:
      LOG(ERROR) << "Consume failed: " << msgWithError.errstr();
      break;
    }
  }

 protected:
  // allow subclasses to override processBatch without exposing the underlying consumer object
  size_t consumeBatch(int timeoutMs, void* opaque) {
    size_t count = 0;
    int64_t start = nowMs();
    int remainingMs = timeoutMs;
    while (run() && count < kMaxBatchSize && remainingMs > 0) {
      std::unique_ptr<RdKafka::Message> msg(consumer_->consume(remainingMs));
      if (!msg) {
        break;
      }
      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        processOne(*msg, opaque);
        count++;
      } else {
        processError(*msg, opaque);
        break;
      }
      remainingMs = timeoutMs - (nowMs() - start);
    }
    return count;
  }

  bool commitSync() {
    auto errorCode = consumer_->commitSync();
    if (errorCode != RdKafka::ERR_NO_ERROR) {
      LOG(WARNING) << "Commit offset sync to broker failed: " << RdKafka::err2str(errorCode);
      return false;
    }

    return true;
  }

  bool commitAsync() {
    auto errorCode = consumer_->commitAsync();
    if (errorCode != RdKafka::ERR_NO_ERROR) {
      LOG(WARNING) << "Commit offset async to broker failed: " << RdKafka::err2str(errorCode);
      return false;
    }

    return true;
  }

  bool commitAsync(int64_t offset) {
    std::vector<RdKafka::TopicPartition*> offsets;

    std::unique_ptr<RdKafka::TopicPartition> partition(RdKafka::TopicPartition::create(topicStr_, partition_));
    partition->set_offset(offset);
    offsets.push_back(partition.get());

    auto errorCode = consumer_->commitAsync(offsets);
    if (errorCode != RdKafka::ERR_NO_ERROR) {
      LOG(WARNING) << "Commit offset async to broker failed: " << RdKafka::err2str(errorCode);
      return false;
    }

    return true;
  }

  // Injecting resources to allow mocking during test
  virtual std::unique_ptr<RdKafka::Topic> createKafkaTopic(RdKafka::KafkaConsumer* consumer,
                                                           const std::string& topicStr, RdKafka::Conf* topicConf,
                                                           std::string* errstr) {
    return std::unique_ptr<RdKafka::Topic>(RdKafka::Topic::create(consumer, topicStr, topicConf, *errstr));
  }
  virtual std::unique_ptr<RdKafka::KafkaConsumer> createKafkaConsumer(RdKafka::Conf* conf, std::string* errstr) {
    return std::unique_ptr<RdKafka::KafkaConsumer>(RdKafka::KafkaConsumer::create(conf, *errstr));
  }

 private:
  static constexpr size_t kMaxBatchSize = 10000;

  void setConf(const std::string& name, const std::string& value) {
    std::string errstr;
    if (conf_->set(name, value, errstr) != RdKafka::Conf::CONF_OK) {
      LOG(FATAL) << "Setting Kafka consumer configuration failed: " << errstr;
    }
  }

  const std::string brokerList_;
  const std::string topicStr_;
  const int partition_;
  const std::string groupId_;
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_CONSUMER_H_
