#include "pipeline/KafkaConsumerConfig.h"

#include <string>
#include <utility>

#include "folly/dynamic.h"
#include "glog/logging.h"
#include "librdkafka/rdkafkacpp.h"

namespace pipeline {

KafkaConsumerConfig KafkaConsumerConfig::createFromJson(const folly::dynamic& config) {
  // Required configs
  const std::string consumerName = CHECK_NOTNULL(config.get_ptr("consumer_name"))->getString();
  const std::string topic = CHECK_NOTNULL(config.get_ptr("topic"))->getString();
  const int partition = CHECK_NOTNULL(config.get_ptr("partition"))->getInt();
  const std::string groupId = CHECK_NOTNULL(config.get_ptr("group_id"))->getString();

  // optional configs
  std::string offsetKeySuffix = "";
  if (config.get_ptr("offset_key_suffix")) {
    offsetKeySuffix = config["offset_key_suffix"].getString();
  }
  bool consumeFromBeginningOneOff = false;
  if (config.get_ptr("consume_from_beginning_one_off")) {
    consumeFromBeginningOneOff = config["consume_from_beginning_one_off"].getBool();
  }
  int64_t initialOffsetOneOff = RdKafka::Topic::OFFSET_INVALID;
  if (config.get_ptr("initial_offset_one_off")) {
    initialOffsetOneOff = config["initial_offset_one_off"].getInt();
  }
  CHECK(!(consumeFromBeginningOneOff && initialOffsetOneOff >= 0))
      << "Cannot defined both consume_from_beginning_one_off and initial_offset_one_off";

  // optional configs for kafka store consumers
  std::string objectStoreBucketName = "";
  if (config.get_ptr("object_store_bucket_name")) {
    objectStoreBucketName = config["object_store_bucket_name"].getString();
  }
  std::string objectStoreObjectNamePrefix = "";
  if (config.get_ptr("object_store_object_name_prefix")) {
    objectStoreObjectNamePrefix = config["object_store_object_name_prefix"].getString();
  }

  return KafkaConsumerConfig(std::move(consumerName), std::move(topic), partition, std::move(groupId),
                             std::move(offsetKeySuffix), consumeFromBeginningOneOff, initialOffsetOneOff,
                             objectStoreBucketName, objectStoreObjectNamePrefix);
}

}  // namespace pipeline
