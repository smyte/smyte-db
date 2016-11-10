#ifndef PIPELINE_KAFKACONSUMERCONFIG_H_
#define PIPELINE_KAFKACONSUMERCONFIG_H_

#include <string>
#include <utility>

#include "folly/dynamic.h"

namespace pipeline {

class KafkaConsumerConfig {
 public:
  // Extract configurations from JSON map.
  static KafkaConsumerConfig createFromJson(const folly::dynamic& config);

  KafkaConsumerConfig(std::string _consumerName, std::string _topic, int _partition, std::string _groupId,
                      std::string _offsetKeySuffix, bool _consumeFromBeginningOneoff, int64_t _initialOffsetOneoff,
                      std::string _objectStoreBucketName, std::string _objectStoreObjectNamePrefix)
      : consumerName(std::move(_consumerName)),
        topic(std::move(_topic)),
        partition(_partition),
        groupId(std::move(_groupId)),
        offsetKeySuffix(std::move(_offsetKeySuffix)),
        consumeFromBeginningOneOff(_consumeFromBeginningOneoff),
        initialOffsetOneOff(_initialOffsetOneoff),
        objectStoreBucketName(_objectStoreBucketName),
        objectStoreObjectNamePrefix(_objectStoreObjectNamePrefix) {}

  const std::string consumerName;
  const std::string topic;
  const int partition;
  const std::string groupId;
  const std::string offsetKeySuffix;
  const bool consumeFromBeginningOneOff;
  const int64_t initialOffsetOneOff;
  const std::string objectStoreBucketName;
  const std::string objectStoreObjectNamePrefix;
};

}  // namespace pipeline

#endif  // PIPELINE_KAFKACONSUMERCONFIG_H_
