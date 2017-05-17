#ifndef INFRA_KAFKA_CONSUMERHELPER_H_
#define INFRA_KAFKA_CONSUMERHELPER_H_

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>

#include "folly/Conv.h"
#include "folly/Format.h"
#include "folly/Range.h"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_batch_base.h"
#include "librdkafka/rdkafkacpp.h"

namespace infra {
namespace kafka {

// Kafka consumer helper provide helper functions to deal with kafka-related operations.
// Common operations include load/store kafka offsets and parse kafka stats JSON.
// Each helper object handles one kafka partition of one topic.
class ConsumerHelper {
 public:
  // Encode 64-bit offset into a byte array suited for writing to persistent key/value stores.
  static std::string encodeOffset(int64_t offset) {
    // use simple string-encoding so that it's easy to inspect the value in redis-cli
    // not really wasting space here since each DB only has at most a handful of such offsets
    return folly::to<std::string>(offset);
  }

  // Decode value into a 64-bit kafka offset
  static int64_t decodeOffset(const std::string& value) {
    try {
      return folly::to<int64_t>(value);
    } catch (std::range_error&) {
      LOG(ERROR) << "Error in decoding kafka offset: " << value;
      return RdKafka::Topic::OFFSET_INVALID;
    }
  }

  // Encode two 64-bit offsets into a byte array suited for writing to persistent key/value stores.
  static std::string encodeKafkaAndFileOffsets(int64_t kafkaOffset, int64_t fileOffset) {
    // kafka store offsets do not support special values that are negative
    CHECK(kafkaOffset >= 0 && fileOffset >= 0);
    // similar to encodeOffset, we improve human readability at the cost of slight inefficiency of string encoding
    return folly::sformat(kKafkaAndFileOffsetsFormat, kafkaOffset, fileOffset);
  }

  // Decode value into two 64-bit offsets
  // Return value indicates if the operation succeeded
  static bool decodeKafkaAndFileOffsets(const std::string& value, int64_t* kafkaOffset, int64_t* fileOffset) {
    // value should contain two encoded 64-bit integers and a `:`
    if (value.size() != kInt64MaxDigits * 2 + 1) {
      LOG(ERROR) << "Encoded kafka and file offsets are not 41 bytes long: " << value;
      return false;
    }

    try {
      if (kafkaOffset) *kafkaOffset = folly::to<int64_t>({value.data(), kInt64MaxDigits});
      if (fileOffset) *fileOffset = folly::to<int64_t>({value.data() + kInt64MaxDigits + 1, kInt64MaxDigits});
    } catch (std::range_error&) {
      LOG(ERROR) << "Error in parsing kafka and file offsets: " << value;
      return false;
    }
    return true;
  }

  ConsumerHelper(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* smyteMetadataCfHandle)
      : db_(db),
        smyteMetadataCfHandle_(smyteMetadataCfHandle),
        topicPartitions_({}),
        lastCommittedOffsets_({}),
        highWatermarkOffsets_({}),
        // consider consumers lagging at start up time until they prove otherwise
        isLagging_(true) {}

  // Commit the given kafka offset regardless of its value, i.e., special negative values are allowed
  bool commitRawOffset(const std::string& offsetKey, int64_t kafkaOffset,
                       rocksdb::WriteBatchBase* writeBatch = nullptr) {
    if (!commitRawOffsetValueWithWriteBatch(offsetKey, encodeOffset(kafkaOffset), writeBatch)) {
      return false;
    }
    setLastCommittedOffset(offsetKey, kafkaOffset);
    return true;
  }

  // Commit the given kafka offset, but only allow non-negative values
  bool commitNextProcessOffset(const std::string& offsetKey, int64_t nextProcessOffset,
                               rocksdb::WriteBatchBase* writeBatch = nullptr) {
    CHECK(nextProcessOffset >= 0) << "Expected non-negative offset to process next";
    if (!commitRawOffsetValueWithWriteBatch(offsetKey, encodeOffset(nextProcessOffset), writeBatch)) {
      return false;
    }
    setLastCommittedOffset(offsetKey, nextProcessOffset);
    return true;
  }

  // Similar to commitNextProcessOffset but also commit file offset.
  bool commitNextProcessKafkaAndFileOffsets(const std::string& offsetKey, int64_t nextProcessOffset,
                                            int64_t fileOffset, rocksdb::WriteBatchBase* writeBatch = nullptr) {
    CHECK(nextProcessOffset >= 0 && fileOffset >= 0) << "Expected non-negative offset to process next";
    if (!commitRawOffsetValueWithWriteBatch(offsetKey, encodeKafkaAndFileOffsets(nextProcessOffset, fileOffset),
                                            writeBatch)) {
      return false;
    }
    setLastCommittedOffset(offsetKey, nextProcessOffset);
    return true;
  }

  bool commitRawKafkaAndFileOffset(const std::string& offsetKey, int64_t kafkaOffset, int64_t fileOffset,
                                   rocksdb::WriteBatchBase* writeBatch = nullptr) {
    if (!commitRawOffsetValueWithWriteBatch(offsetKey, encodeKafkaAndFileOffsets(kafkaOffset, fileOffset),
                                            writeBatch)) {
      return false;
    }
    setLastCommittedOffset(offsetKey, kafkaOffset);
    return true;
  }

  std::string getOffsetKey(const std::string& topic, int partition, const std::string& offsetKeySuffix) {
    return folly::sformat("~kafka-offset~{}~{}~{}", topic, partition, offsetKeySuffix);
  }

  // Support a new topic/partition pair and return a offset key with the given suffix
  std::string linkTopicPartition(const std::string& topic, int partition, const std::string& offsetKeySuffix) {
    auto offsetKey = getOffsetKey(topic, partition, offsetKeySuffix);
    const auto it = topicPartitions_.find(offsetKey);
    CHECK(it == topicPartitions_.end()) << "Topic " << topic << " partition " << partition << " already linked";

    topicPartitions_[offsetKey] = std::make_pair(topic, partition);
    lastCommittedOffsets_[offsetKey] = RdKafka::Topic::OFFSET_INVALID;
    highWatermarkOffsets_[offsetKey] = RdKafka::Topic::OFFSET_INVALID;
    // consider consumers lagging at start up time until they prove otherwise
    lagStatuses[offsetKey] = true;
    return offsetKey;
  }

  // Load kafka offset from rocksdb
  int64_t loadCommittedOffsetFromDb(const std::string& offsetKey);

  bool loadCommittedKafkaAndFileOffsetsFromDb(const std::string& offsetKey, int64_t* kafkaOffset, int64_t* fileOffset);

  // Parse the latest copy of kafka stats, which is JSON-encoded string, to update internal stats
  void updateStats(const std::string& statsJson, const std::string& offsetKey) {
    const auto it = topicPartitions_.find(offsetKey);
    CHECK(it != topicPartitions_.end());
    int64_t newOffset = parseHighWatermarkOffset(statsJson, it->second.first, it->second.second);
    if (LIKELY(newOffset != RdKafka::Topic::OFFSET_INVALID)) {
      setHighWatermarkOffset(offsetKey, newOffset);
    }
  }

  // Output kafka consumer stats in redis info format
  void appendStatsInRedisInfoFormat(std::stringstream* ss) const {
    for (const auto& entry : topicPartitions_) {
      std::string prefix = folly::sformat("kafka_topic_{}_partition_{}_", entry.second.first, entry.second.second);
      int64_t lastCommittedOffset = getLastCommittedOffset(entry.first);
      int64_t highWatermarkOffset = getHighWatermarkOffset(entry.first);
      (*ss) << prefix << "last_committed_offset:" << lastCommittedOffset << std::endl;
      (*ss) << prefix << "high_watermark_offset:" << highWatermarkOffset << std::endl;
      (*ss) << prefix << "lag:" << std::max(0L, highWatermarkOffset - lastCommittedOffset) << std::endl;
    }
    (*ss) << "is_any_consumer_lagging:" << isLagging() << std::endl;
  }

  int64_t getLastCommittedOffset(const std::string& offsetKey) const {
    const auto it = lastCommittedOffsets_.find(offsetKey);
    CHECK(it != lastCommittedOffsets_.end());
    return it->second;
  }

  int64_t getHighWatermarkOffset(const std::string& offsetKey) const {
    const auto it = highWatermarkOffsets_.find(offsetKey);
    CHECK(it != highWatermarkOffsets_.end());
    return it->second;
  }

  int64_t setLastCommittedOffset(const std::string& offsetKey, int64_t offset) {
    const auto it = lastCommittedOffsets_.find(offsetKey);
    CHECK(it != lastCommittedOffsets_.end());
    it->second = offset;
    return offset;
  }

  int64_t setHighWatermarkOffset(const std::string& offsetKey, int64_t offset) {
    const auto it = highWatermarkOffsets_.find(offsetKey);
    CHECK(it != highWatermarkOffsets_.end());
    it->second = offset;
    return offset;
  }

  // Mark the consumer for the given key not lagging
  void setNoLag(const std::string& offsetKey) {
    // Currently we only evaluate if consumers are lagging after start up.
    // Once they caught up, we no longer check it
    if (!isLagging_) return;

    const auto it = lagStatuses.find(offsetKey);
    CHECK(it != lagStatuses.end());
    it->second = false;

    // Need to check if we are still lagging
    // It may seem like a slow loop but it's not in practice because
    // a) The total number of entries is small (mostly 1 or 2 and very occasionally 8)
    // b) It is only evaluated after start up and until everyone catches up
    for (const auto& entry : lagStatuses) {
      if (entry.second) {
        // it is still lagging
        return;
      }
    }
    // No longer lagging!
    // NOTE: no mutex needed here because we only ever set it to false and there is write after read
    isLagging_ = false;
  }

  // Overwrite the lag status for individual consumers
  void setLagStatus(bool isLagging) {
    isLagging_ = isLagging;
  }

  bool isLagging() const { return isLagging_; }

 private:
  using TopicPartition = std::pair<std::string, int64_t>;

  // constants needed for fixed-length string encoding of int64_t
  static constexpr int kInt64MaxDigits = 20;
  static constexpr char kKafkaAndFileOffsetsFormat[] = "{:020d}:{:020d}";

  // Commit offset to rocksdb using a write batch, which allows the caller to persist other data atomically.
  bool commitRawOffsetValueWithWriteBatch(const std::string& offsetKey, const std::string& offsetValue,
                                          rocksdb::WriteBatchBase* writeBatch = nullptr);

  int64_t parseHighWatermarkOffset(const std::string& statsJson, const std::string& topic, int64_t partition);

  rocksdb::DB* db_;
  rocksdb::ColumnFamilyHandle* smyteMetadataCfHandle_;

  // Use offset key to index various maps
  // Note: No locks need to protect the maps. We only add elements sequentially during initialization time.
  // After that, we only set/load existing elements indirectly with via const methods
  std::map<std::string, TopicPartition> topicPartitions_;
  std::map<std::string, int64_t> lastCommittedOffsets_;
  std::map<std::string, int64_t> highWatermarkOffsets_;
  std::map<std::string, bool> lagStatuses;
  // true if any consumer is lagging
  bool isLagging_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_CONSUMERHELPER_H_
