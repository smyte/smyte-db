#ifndef INFRA_KAFKA_OFFSETMANAGER_H_
#define INFRA_KAFKA_OFFSETMANAGER_H_

#include <memory>
#include <string>

#include "glog/logging.h"
#include "librdkafka/rdkafka.h"

namespace infra {
namespace kafka {

// Use low-level C APIs in librdkafka to commit/fetch offsets for a group directly without consuming any message.
// The offset to be committed doesn't have to be a valid offset that exists in the brokers for the given topic.
class OffsetManager {
 public:
  OffsetManager(const std::string& brokerList, const std::string& topic, int partition, const std::string& groupId)
      : brokerList_(brokerList),
        topic_(topic),
        partition_(partition),
        groupId_(groupId),
        consumer_(nullptr, rd_kafka_destroy),
        topicPartitions_(rd_kafka_topic_partition_list_new(1), rd_kafka_topic_partition_list_destroy) {}

  ~OffsetManager() {
    rd_kafka_consumer_close(consumer_.get());
  }

  // Initialize offset manager. Panic on errors/failures.
  void init(void);

  // Fetch committed offset from broker. Return value indicates if the operation succeeded.
  bool committedOffset(int timeoutMs, int64_t* offset);

  // Commit offset to broker. Return value indicates if the operation succeeded.
  // async argument determines if the operation is blocking. It is sync by default.
  bool commitOffset(int64_t offset, bool async = false);

 private:
  static void setConf(const char name[], const char value[], rd_kafka_conf_t* conf) {
    char errstr[512];
    if (rd_kafka_conf_set(conf, name, value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
      LOG(FATAL) << "Setting Kafka consumer configuration failed: " << errstr;
    }
  }

  const std::string brokerList_;
  const std::string topic_;
  const int partition_;
  const std::string groupId_;
  std::unique_ptr<rd_kafka_t, void(*)(rd_kafka_t *)> consumer_;
  std::unique_ptr<rd_kafka_topic_partition_list_t, void(*)(rd_kafka_topic_partition_list_t *)> topicPartitions_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_OFFSETMANAGER_H_
