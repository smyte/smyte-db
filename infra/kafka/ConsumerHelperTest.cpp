#include <limits>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "infra/kafka/ConsumerHelper.h"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "stesting/TestWithRocksDb.h"

namespace infra {
namespace kafka {

// db() is automatically available using this helper class
class ConsumerHelperTest : public stesting::TestWithRocksDb {};

TEST_F(ConsumerHelperTest, EncodeDecodeOffset) {
  // offset 475
  std::string value = ConsumerHelper::encodeOffset(475);
  EXPECT_EQ("475", value);
  EXPECT_EQ(475, ConsumerHelper::decodeOffset(value));

  // offset 0
  value = ConsumerHelper::encodeOffset(0);
  EXPECT_EQ("0", value);
  EXPECT_EQ(0, ConsumerHelper::decodeOffset(value));

  // offset -1000
  value = ConsumerHelper::encodeOffset(-1000);
  EXPECT_EQ("-1000", value);
  EXPECT_EQ(-1000, ConsumerHelper::decodeOffset(value));

  // offset 2 ^ 63 - 1
  value = ConsumerHelper::encodeOffset(std::numeric_limits<int64_t>::max());
  EXPECT_EQ("9223372036854775807", value);
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), ConsumerHelper::decodeOffset(value));

  // offset - 2 ^ 64
  value = ConsumerHelper::encodeOffset(std::numeric_limits<int64_t>::min());
  EXPECT_EQ("-9223372036854775808", value);
  EXPECT_EQ(std::numeric_limits<int64_t>::min(), ConsumerHelper::decodeOffset(value));

  // decode invalid offset
  EXPECT_TRUE(ConsumerHelper::decodeOffset("abc") == RdKafka::Topic::OFFSET_INVALID);
}

TEST_F(ConsumerHelperTest, EncodeDecodeKafkaAndFileOffsets) {
  // offset 475, 123
  std::string value = ConsumerHelper::encodeKafkaAndFileOffsets(475, 123);
  EXPECT_EQ("00000000000000000475:00000000000000000123", value);
  int64_t kafkaOffset, fileOffset;
  EXPECT_TRUE(ConsumerHelper::decodeKafkaAndFileOffsets(value, nullptr, &fileOffset));
  EXPECT_EQ(123, fileOffset);
  EXPECT_TRUE(ConsumerHelper::decodeKafkaAndFileOffsets(value, &kafkaOffset, nullptr));
  EXPECT_EQ(475, kafkaOffset);
  EXPECT_TRUE(ConsumerHelper::decodeKafkaAndFileOffsets(value, &kafkaOffset, &fileOffset));
  EXPECT_EQ(475, kafkaOffset);
  EXPECT_EQ(123, fileOffset);

  // offset 0 and 2 ^ 63 - 1
  value = ConsumerHelper::encodeKafkaAndFileOffsets(0, std::numeric_limits<int64_t>::max());
  EXPECT_EQ("00000000000000000000:09223372036854775807", value);
  EXPECT_TRUE(ConsumerHelper::decodeKafkaAndFileOffsets(value, &kafkaOffset, &fileOffset));
  EXPECT_EQ(0, kafkaOffset);
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), fileOffset);

  // decode invalid offsets
  EXPECT_FALSE(ConsumerHelper::decodeKafkaAndFileOffsets("001002", &kafkaOffset, &fileOffset));
  EXPECT_FALSE(ConsumerHelper::decodeKafkaAndFileOffsets("00000000000000000000:0922337203685477580a", &kafkaOffset,
                                                         &fileOffset));
}

TEST_F(ConsumerHelperTest, OffsetKey) {
  ConsumerHelper consumerHelper(db(), metadataColumnFamily());
  EXPECT_EQ("~kafka-offset~testTopic1~1~", consumerHelper.linkTopicPartition("testTopic1", 1, ""));
  EXPECT_EQ("~kafka-offset~testTopic2~2~day", consumerHelper.linkTopicPartition("testTopic2", 2, "day"));
}

TEST_F(ConsumerHelperTest, SaveLoadOffset) {
  ConsumerHelper consumerHelper(db(), metadataColumnFamily());
  const std::string offsetKey = consumerHelper.linkTopicPartition("testTopic", 1, "");

  rocksdb::WriteBatch writeBatch;
  EXPECT_TRUE(consumerHelper.commitNextProcessOffset(offsetKey, 105, &writeBatch));
  EXPECT_EQ(105, consumerHelper.getLastCommittedOffset(offsetKey));
  EXPECT_EQ(105, consumerHelper.loadCommittedOffsetFromDb(offsetKey));
}

TEST_F(ConsumerHelperTest, SaveLoadKafkaAndFileOffsets) {
  ConsumerHelper consumerHelper(db(), metadataColumnFamily());
  const std::string offsetKey = consumerHelper.linkTopicPartition("testTopic", 1, "");

  rocksdb::WriteBatch writeBatch;
  EXPECT_TRUE(consumerHelper.commitNextProcessKafkaAndFileOffsets(offsetKey, 105, 95, &writeBatch));
  EXPECT_EQ(105, consumerHelper.getLastCommittedOffset(offsetKey));
  int64_t kafkaOffset, fileOffset;
  EXPECT_TRUE(consumerHelper.loadCommittedKafkaAndFileOffsetsFromDb(offsetKey, &kafkaOffset, &fileOffset));
  EXPECT_EQ(105, kafkaOffset);
  EXPECT_EQ(95, fileOffset);
}

TEST_F(ConsumerHelperTest, UpdateStats) {
  ConsumerHelper consumerHelper(db(), metadataColumnFamily());
  const std::string offsetKey = consumerHelper.linkTopicPartition("testTopic", 1, "");
  consumerHelper.setHighWatermarkOffset(offsetKey, 101);

  // no change to the high watermark offset when parsing failed
  consumerHelper.updateStats(R"({ "name": "invalid kafka stats JSON" })", offsetKey);
  EXPECT_EQ(101, consumerHelper.getHighWatermarkOffset(offsetKey));

  // update high watermark offset when parsing succeeded
  consumerHelper.updateStats(R"({ "topics": { "testTopic": { "partitions" : { "1": { "hi_offset": 109 } } } } })",
                             offsetKey);
  EXPECT_EQ(109, consumerHelper.getHighWatermarkOffset(offsetKey));
}

TEST_F(ConsumerHelperTest, AppendStatsInRedisInfoFormat) {
  ConsumerHelper consumerHelper(db(), metadataColumnFamily());
  const std::string offsetKey1 = consumerHelper.linkTopicPartition("testTopic1", 1, "");
  const std::string offsetKey2 = consumerHelper.linkTopicPartition("testTopic2", 0, "");

  // normal case
  std::stringstream actual;
  std::stringstream expected;
  consumerHelper.setLastCommittedOffset(offsetKey1, 100);
  consumerHelper.setHighWatermarkOffset(offsetKey1, 115);
  consumerHelper.setLastCommittedOffset(offsetKey2, 200);
  consumerHelper.setHighWatermarkOffset(offsetKey2, 230);
  consumerHelper.appendStatsInRedisInfoFormat(&actual);
  expected << "kafka_topic_testTopic1_partition_1_last_committed_offset:100" << std::endl
           << "kafka_topic_testTopic1_partition_1_high_watermark_offset:115" << std::endl
           << "kafka_topic_testTopic1_partition_1_lag:15" << std::endl
           << "kafka_topic_testTopic2_partition_0_last_committed_offset:200" << std::endl
           << "kafka_topic_testTopic2_partition_0_high_watermark_offset:230" << std::endl
           << "kafka_topic_testTopic2_partition_0_lag:30" << std::endl;
  EXPECT_EQ(expected.str(), actual.str());

  // when high watermark offset is smaller (yet to update)
  actual.str("");
  actual.clear();
  expected.str("");
  expected.clear();
  consumerHelper.setLastCommittedOffset(offsetKey1, 100);
  consumerHelper.setHighWatermarkOffset(offsetKey1, 95);
  consumerHelper.setLastCommittedOffset(offsetKey2, 200);
  consumerHelper.setHighWatermarkOffset(offsetKey2, 230);
  consumerHelper.appendStatsInRedisInfoFormat(&actual);
  expected << "kafka_topic_testTopic1_partition_1_last_committed_offset:100" << std::endl
           << "kafka_topic_testTopic1_partition_1_high_watermark_offset:95" << std::endl
           << "kafka_topic_testTopic1_partition_1_lag:0" << std::endl
           << "kafka_topic_testTopic2_partition_0_last_committed_offset:200" << std::endl
           << "kafka_topic_testTopic2_partition_0_high_watermark_offset:230" << std::endl
           << "kafka_topic_testTopic2_partition_0_lag:30" << std::endl;
  EXPECT_EQ(expected.str(), actual.str());
}

}  // namespace kafka
}  // namespace infra
