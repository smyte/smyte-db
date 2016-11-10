#include "pipeline/KafkaConsumerConfig.h"

#include "folly/dynamic.h"
#include "gtest/gtest.h"
#include "librdkafka/rdkafkacpp.h"

namespace pipeline {

TEST(KafkaConsumerConfig, CreateFromJsonMissingRequired) {
  EXPECT_DEATH(KafkaConsumerConfig::createFromJson(folly::dynamic::object("topic", "abc")), "Check failed.*non NULL");
}

TEST(KafkaConsumerConfig, CreateFromJsonDefaultOptionalFields) {
  auto config = KafkaConsumerConfig::createFromJson(
      folly::dynamic::object("consumer_name", "TestConsumer")("topic", "abc")("partition", 1)("group_id", "test"));
  EXPECT_TRUE(config.offsetKeySuffix.empty());
  EXPECT_FALSE(config.consumeFromBeginningOneOff);
  EXPECT_TRUE(RdKafka::Topic::OFFSET_INVALID == config.initialOffsetOneOff);
  EXPECT_TRUE(config.objectStoreBucketName.empty());
  EXPECT_TRUE(config.objectStoreObjectNamePrefix.empty());
}

TEST(KafkaConsumerConfig, CreateFromJsonConflictingOffsets) {
  EXPECT_DEATH(KafkaConsumerConfig::createFromJson(
    folly::dynamic::object
      ("consumer_name", "TestConsumer")
      ("topic", "abc")
      ("partition", 1)
      ("group_id", "test")
      ("offset_key_suffix", "day")
      ("initial_offset_one_off", 123)
      ("consume_from_beginning_one_off", true)),
    "Check failed.*Cannot defined both consume_from_beginning_one_off and initial_offset_one_off");
}

TEST(KafkaConsumerConfig, CreateFromJsonOptionalFields) {
  auto config = KafkaConsumerConfig::createFromJson(folly::dynamic::object
      ("consumer_name", "TestConsumer")
      ("topic", "abc")
      ("partition", 1)
      ("group_id", "test")
      ("offset_key_suffix", "day")
      ("initial_offset_one_off", 123)
      ("object_store_bucket_name", "kafka")
      ("object_store_object_name_prefix", "raw/"));
  EXPECT_EQ("day", config.offsetKeySuffix);
  EXPECT_FALSE(config.consumeFromBeginningOneOff);
  EXPECT_EQ(123L, config.initialOffsetOneOff);
  EXPECT_EQ("kafka", config.objectStoreBucketName);
  EXPECT_EQ("raw/", config.objectStoreObjectNamePrefix);
}

}  // namespace pipeline
