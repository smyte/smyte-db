#include "infra/kafka/OffsetManager.h"

#include <csignal>
#include <memory>

#include "folly/Conv.h"
#include "folly/Format.h"
#include "glog/logging.h"
#include "librdkafka/rdkafka.h"

namespace infra {
namespace kafka {

void OffsetManager::init(void) {
  auto conf = std::unique_ptr<rd_kafka_conf_t, void(*)(rd_kafka_conf_t *)>(rd_kafka_conf_new(), rd_kafka_conf_destroy);
  setConf("client.id", folly::sformat("cpp_store_client_{}_{}", topic_, partition_).data(), conf.get());
  CHECK(!groupId_.empty());
  setConf("group.id", groupId_.data(), conf.get());
  setConf("metadata.broker.list", brokerList_.data(), conf.get());
  setConf("socket.keepalive.enable", "true", conf.get());
  // Disable verbose logging
  setConf("log.connection.close", "false", conf.get());
  // For quick termination. See official librdkafka docs/examples
  setConf("internal.termination.signal", folly::to<std::string>(SIGIO).data(), conf.get());

  char errstr[512];
  // consumer_ now owns conf
  consumer_.reset(rd_kafka_new(RD_KAFKA_CONSUMER, conf.release(), errstr, sizeof(errstr)));
  CHECK(consumer_) << "Failed to create consumer: " << errstr;

  CHECK(topicPartitions_) << "Failed to create topic partition list";
  CHECK(rd_kafka_topic_partition_list_add(topicPartitions_.get(), topic_.data(), partition_))
      << "Failed to add topic " << topic_ << " partition " << partition_ << " to the list";

  // Fetch the offset to make sure connection works.
  int64_t offset = RD_KAFKA_OFFSET_INVALID;
  CHECK(committedOffset(5000, &offset));
  LOG(INFO) << "Last committed offset was " << offset;
}

bool OffsetManager::committedOffset(int timeoutMs, int64_t* offset) {
  auto err = rd_kafka_committed(consumer_.get(), topicPartitions_.get(), timeoutMs);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG(ERROR) << "Getting committed offset failed: " << rd_kafka_err2str(err);
    return false;
  }

  // topicPartitions_ still owns the pointer, so no need to free it
  const rd_kafka_topic_partition_t* topicPartition =
      rd_kafka_topic_partition_list_find(topicPartitions_.get(), topic_.data(), partition_);
  CHECK_NOTNULL(topicPartition);
  if (topicPartition->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG(ERROR) << "Getting committed offset failed: " << rd_kafka_err2str(topicPartition->err);
    return false;
  }

  *offset = topicPartition->offset;
  return true;
}

bool OffsetManager::commitOffset(int64_t offset, bool async) {
  auto err = rd_kafka_topic_partition_list_set_offset(topicPartitions_.get(), topic_.data(), partition_, offset);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG(ERROR) << "Setting offset failed: " << rd_kafka_err2str(err);
    return false;
  }
  err = rd_kafka_commit(consumer_.get(), topicPartitions_.get(), async);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG(ERROR) << "Committing offset failed: " << rd_kafka_err2str(err);
    return false;
  }
  return true;
}

}  // namespace kafka
}  // namespace infra
