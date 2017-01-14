#include "infra/kafka/ConsumerTest.h"

#include <memory>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "librdkafka/rdkafkacpp.h"

DECLARE_bool(logtostderr);

namespace infra {
namespace kafka {

class ConsumerTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    FLAGS_logtostderr = true;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging("infra::kafka::ConsumerTest");
  }
};

using ::testing::_;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Return;

TEST_F(ConsumerTest, ConsumeFromBeginning) {
  // These raw pointers are owned by MockConsumer, which will be deleted automatically
  MockKafkaTopic* kafkaTopic = new MockKafkaTopic();
  MockKafkaMetadata* kafkaMetadata = new MockKafkaMetadata();
  MockKafkaConsumer* kafkaConsumer = new MockKafkaConsumer();

  MockConsumer consumer("localhost:9092", "testTopic", 0, kafkaTopic, kafkaConsumer);

  // setup metadata
  auto topicMetadata = std::make_unique<MockKafkaTopicMetadata>();
  auto partitionMetadata = std::make_unique<MockPartitionMetadata>();
  EXPECT_CALL(*topicMetadata, topic())
      .WillOnce(Return("testTopic"));
  EXPECT_CALL(*topicMetadata, partitions())
      .WillOnce(Return(new RdKafka::TopicMetadata::PartitionMetadataVector{partitionMetadata.get()}));
  EXPECT_CALL(*kafkaMetadata, topics())
      .WillOnce(Return(new RdKafka::Metadata::TopicMetadataVector{topicMetadata.get()}));
  EXPECT_CALL(*kafkaConsumer, metadata(false, kafkaTopic, _, 10000))
      .WillOnce(DoAll(SetArgPointee<2>(kafkaMetadata), Return(RdKafka::ERR_NO_ERROR)));

  EXPECT_CALL(*kafkaConsumer, name())
      .WillOnce(Return("MockKafkaConsumer"));
  EXPECT_CALL(*kafkaConsumer, assign(_))
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  EXPECT_CALL(*kafkaConsumer, close())
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  // Never calls loadCommittedKafkaOffset, so use RdKafka::Topic::OFFSET_BEGINNING by default
  EXPECT_CALL(consumer, loadCommittedKafkaOffset())
      .Times(0);

  // calls stop before start to make sure we don't actually start consuming but testing the initialization part only
  consumer.stop();
  consumer.init(RdKafka::Topic::OFFSET_BEGINNING);
  consumer.start(1000);
  consumer.destroy();
}

TEST_F(ConsumerTest, ConsumeUsingStoredOffset) {
  // These raw pointers are owned by MockConsumer, which will be deleted automatically
  MockKafkaTopic* kafkaTopic = new MockKafkaTopic();
  MockKafkaMetadata* kafkaMetadata = new MockKafkaMetadata();
  MockKafkaConsumer* kafkaConsumer = new MockKafkaConsumer();

  MockConsumer consumer("localhost:9092", "testTopic", 0, kafkaTopic, kafkaConsumer);

  // setup metadata
  auto topicMetadata = std::make_unique<MockKafkaTopicMetadata>();
  auto partitionMetadata = std::make_unique<MockPartitionMetadata>();
  EXPECT_CALL(*topicMetadata, topic())
      .WillOnce(Return("testTopic"));
  EXPECT_CALL(*topicMetadata, partitions())
      .WillOnce(Return(new RdKafka::TopicMetadata::PartitionMetadataVector{partitionMetadata.get()}));
  EXPECT_CALL(*kafkaMetadata, topics())
      .WillOnce(Return(new RdKafka::Metadata::TopicMetadataVector{topicMetadata.get()}));
  EXPECT_CALL(*kafkaConsumer, metadata(false, kafkaTopic, _, 10000))
      .WillOnce(DoAll(SetArgPointee<2>(kafkaMetadata), Return(RdKafka::ERR_NO_ERROR)));

  EXPECT_CALL(*kafkaConsumer, name())
      .WillOnce(Return("MockKafkaConsumer"));
  EXPECT_CALL(*kafkaConsumer, assign(_))
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  EXPECT_CALL(*kafkaConsumer, close())
      .WillOnce(Return(RdKafka::ERR_NO_ERROR));
  // Returns 10 as stored offset, which is used later by kafkaConsumer->start
  EXPECT_CALL(consumer, loadCommittedKafkaOffset())
      .WillOnce(Return(10));

  // calls stop before start to make sure we don't actual start consuming but testing the initialization part only
  consumer.stop();
  consumer.init(RdKafka::Topic::OFFSET_STORED);
  consumer.start(1000);
  consumer.destroy();
}

}  // namespace kafka
}  // namespace infra
