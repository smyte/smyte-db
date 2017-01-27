#ifndef INFRA_KAFKA_CONSUMERTEST_H_
#define INFRA_KAFKA_CONSUMERTEST_H_

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "infra/kafka/Consumer.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

class MockKafkaTopic : public RdKafka::Topic {
  MOCK_CONST_METHOD0(name, const std::string());
  MOCK_CONST_METHOD1(partition_available, bool(int32_t));
  MOCK_METHOD2(offset_store, RdKafka::ErrorCode(int32_t, int64_t));
};

class MockKafkaMetadata : public RdKafka::Metadata {
 public:
  MOCK_CONST_METHOD0(brokers, const RdKafka::Metadata::BrokerMetadataVector*());
  MOCK_CONST_METHOD0(topics, const RdKafka::Metadata::TopicMetadataVector*());
  MOCK_CONST_METHOD0(orig_broker_name, const std::string());
  MOCK_CONST_METHOD0(orig_broker_id, int32_t());
};

class MockKafkaTopicMetadata : public RdKafka::TopicMetadata {
 public:
  MOCK_CONST_METHOD0(topic, const std::string());
  MOCK_CONST_METHOD0(partitions, const RdKafka::TopicMetadata::PartitionMetadataVector*());
  MOCK_CONST_METHOD0(err, RdKafka::ErrorCode());
};

class MockPartitionMetadata : public RdKafka::PartitionMetadata {
 public:
  MOCK_CONST_METHOD0(id, int32_t());
  MOCK_CONST_METHOD0(err, RdKafka::ErrorCode());
  MOCK_CONST_METHOD0(leader, int32_t());
  MOCK_CONST_METHOD0(replicas, const std::vector<int32_t>*());
  MOCK_CONST_METHOD0(isrs, const std::vector<int32_t>*());
};

class MockKafkaConsumer : public RdKafka::KafkaConsumer {
 public:
  MOCK_CONST_METHOD0(name, const std::string());
  MOCK_CONST_METHOD0(memberid, const std::string());
  MOCK_METHOD1(poll, int(int));
  MOCK_METHOD0(outq_len, int());
  MOCK_METHOD0(close, RdKafka::ErrorCode());
  MOCK_METHOD4(metadata, RdKafka::ErrorCode(bool, const RdKafka::Topic*, RdKafka::Metadata**, int timeout_ms));
  MOCK_METHOD1(pause, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD1(resume, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD5(query_watermark_offsets, RdKafka::ErrorCode(const std::string&, int32_t, int64_t*, int64_t*, int));
  MOCK_METHOD4(get_watermark_offsets, RdKafka::ErrorCode(const std::string&, int32_t, int64_t*, int64_t*));
  MOCK_METHOD1(consume, RdKafka::Message*(int));
  MOCK_METHOD1(assign, RdKafka::ErrorCode(const std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD1(assignment, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD0(unassign, RdKafka::ErrorCode());
  MOCK_METHOD1(position, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD1(subscription, RdKafka::ErrorCode(std::vector<std::string>&));
  MOCK_METHOD1(subscribe, RdKafka::ErrorCode(const std::vector<std::string>&));
  MOCK_METHOD0(unsubscribe, RdKafka::ErrorCode());
  MOCK_METHOD0(commitSync, RdKafka::ErrorCode());
  MOCK_METHOD0(commitAsync, RdKafka::ErrorCode());
  MOCK_METHOD1(commitSync, RdKafka::ErrorCode(RdKafka::Message*));
  MOCK_METHOD1(commitAsync, RdKafka::ErrorCode(RdKafka::Message*));
  MOCK_METHOD1(commitSync, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD1(commitAsync, RdKafka::ErrorCode(const std::vector<RdKafka::TopicPartition*>&));
  MOCK_METHOD2(committed, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&, int));
  MOCK_METHOD1(get_partition_queue, RdKafka::Queue*(const RdKafka::TopicPartition*));
  MOCK_METHOD1(set_log_queue, RdKafka::ErrorCode(RdKafka::Queue*));
  MOCK_METHOD2(offsetsForTimes, RdKafka::ErrorCode(std::vector<RdKafka::TopicPartition*>&, int));
};

class MockConsumer : public Consumer {
 public:
  MockConsumer(std::string brokerList, std::string topicStr, int partition, MockKafkaTopic* kafkaTopic,
               MockKafkaConsumer* kafkaConsumer)
      : infra::kafka::Consumer(brokerList, topicStr, partition, "infra-kafka-consumer-test", "test-key", false,
                               nullptr),
        kafkaTopic_(kafkaTopic),
        kafkaConsumer_(kafkaConsumer) {}

  MOCK_METHOD2(processOne, void(const RdKafka::Message& msg, void* opaque));
  MOCK_METHOD0(loadCommittedKafkaOffset, int64_t());

 protected:
  std::unique_ptr<RdKafka::Topic> createKafkaTopic(RdKafka::KafkaConsumer* consumer, const std::string& topicStr,
                                                   RdKafka::Conf* topicConf, std::string* errstr) override {
    return std::unique_ptr<RdKafka::Topic>(kafkaTopic_);
  }
  std::unique_ptr<RdKafka::KafkaConsumer> createKafkaConsumer(RdKafka::Conf* conf, std::string* errstr) override {
    return std::unique_ptr<RdKafka::KafkaConsumer>(kafkaConsumer_);
  }

 private:
  MockKafkaTopic* kafkaTopic_;
  MockKafkaConsumer* kafkaConsumer_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_CONSUMERTEST_H_
