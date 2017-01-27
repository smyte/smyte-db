#include "infra/kafka/Consumer.h"

#include <memory>
#include <string>
#include <thread>

#include "folly/Format.h"
#include "glog/logging.h"

namespace infra {
namespace kafka {

void Consumer::init(int64_t initialOffset) {
  if (initialOffset == RdKafka::Topic::OFFSET_STORED) {
    initialOffset = loadCommittedKafkaOffset();
  }
  CHECK(initialOffset != RdKafka::Topic::OFFSET_INVALID);

  // set client id unique to the client
  setConf("client.id", folly::sformat("cpp_client_{}_{}", topicStr_, partition_));
  // set consumer group id
  CHECK(!groupId_.empty());
  setConf("group.id", groupId_);
  // seed brokers
  setConf("metadata.broker.list", brokerList_);
  // disable automatic offset commit to improve reliability
  setConf("enable.auto.commit", "false");
  // use ~1M message size explicitly
  setConf("message.max.bytes", "1000000");
  // keepalive for the TCP socket
  setConf("socket.keepalive.enable", "true");
  // disable verbose logging
  setConf("log.connection.close", "false");
  // emit stats every 5 seconds
  setConf("statistics.interval.ms", "5000");
  // requires 0.10.0.0 kafka broker
  setConf("api.version.request", "true");
  // whether to use low latency mode at the cost of throughput
  if (lowLatency_) {
    setConf("fetch.error.backoff.ms", "5");
    setConf("fetch.wait.max.ms", "5");
  }

  std::string errstr;

  // Because this any error in configuration will prevent the consumer from starting, we choose to terminate
  // the program when errors emerge. This is fine because this method is expected be call in the main thread
  // when setting up the whole program
  if (conf_->set("event_cb", static_cast<RdKafka::EventCb*>(this), errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting Kafka event callback failed: " << errstr;
  }

  consumer_ = createKafkaConsumer(conf_.get(), &errstr);
  if (!consumer_) {
    LOG(FATAL) << "Kafka consumer initialization failed: " << errstr;
  }

  LOG(INFO) << "Kafka consumer created: " << consumer_->name();

  // return error when offset is out of range
  std::unique_ptr<RdKafka::Conf> topicConf(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  if (topicConf->set("auto.offset.reset", "error", errstr) != RdKafka::Conf::CONF_OK) {
    LOG(FATAL) << "Setting configuration failed for topic " << topicStr_ << ": " << errstr;
  }

  std::unique_ptr<RdKafka::Topic> topic(createKafkaTopic(consumer_.get(), topicStr_, topicConf.get(), &errstr));
  if (!topic) {
    LOG(FATAL) << "Failed to set Kafka topic: " << topicStr_ << ": " << errstr;
  }

  RdKafka::Metadata* metadataTmp = nullptr;
  auto errorCode = consumer_->metadata(false /* one topic only */, topic.get(), &metadataTmp, 10000);
  std::unique_ptr<RdKafka::Metadata> metadata(metadataTmp);
  if (errorCode != RdKafka::ERR_NO_ERROR) {
    LOG(FATAL) << "Getting topic metadata failed: " << RdKafka::err2str(errorCode);
  }
  bool partitionExists = false;
  for (const auto& topicIt : *metadata->topics()) {
    if (topicIt->topic() == topicStr_) {
      if (partition_ >= 0 && partition_ < static_cast<int>(topicIt->partitions()->size())) {
        partitionExists = true;
      }
    }
  }
  CHECK(partitionExists) << "Partition " << partition_ << " of topic " << topicStr_ << " does not exist";

  const std::string consumerInfoMsg =
      folly::sformat("Start consuming partition {} of {} as {}", partition_, topicStr_, groupId_);
  if (initialOffset == RdKafka::Topic::OFFSET_BEGINNING || initialOffset == RdKafka::Topic::OFFSET_END) {
    LOG(INFO) << consumerInfoMsg << " from the "
              << (initialOffset == RdKafka::Topic::OFFSET_BEGINNING ? "beginning" : "end");
  } else {
    LOG(INFO) << consumerInfoMsg << " from offset " << initialOffset;
  }

  std::unique_ptr<RdKafka::TopicPartition> topicPartiton(RdKafka::TopicPartition::create(topicStr_, partition_));
  topicPartiton->set_offset(initialOffset);
  CHECK_EQ(topicPartiton->err(), RdKafka::ERR_NO_ERROR);

  // assign the partition
  if (consumer_->assign({ topicPartiton.get() }) != RdKafka::ERR_NO_ERROR) {
    LOG(FATAL) << "Assign topic partition failed: " << RdKafka::err2str(errorCode);
  }
  setInitialized();
}

constexpr size_t Consumer::kMaxBatchSize;

}  // namespace kafka
}  // namespace infra
