#ifndef INFRA_SMYTEID_H_
#define INFRA_SMYTEID_H_

#include <chrono>
#include <string>

#include "boost/endian/buffers.hpp"
#include "glog/logging.h"

namespace infra {

class SmyteId {
 public:
  // Total 64 bits for smyte id: Sign:1 TimeMs:40 Unique:10 Machine:13
  // 40 bits local machine increment
  static constexpr int kTimestampBits = 40;
  static constexpr int64_t kTimestampSize = 1L << kTimestampBits;
  static constexpr int64_t kTimestampEpoch = 1262304000000L;  // 2010-01-01

  // Sanity check
  static constexpr int64_t kTimestampSaneEarliest = 1416441600000L;  // 2014-11-20
  static constexpr int64_t kTimestampSaneFutureMs = 1000L * 3600L * 24L * 90L;  // 90 days

  // 10 bits local machine increment
  static constexpr int kUniqueBits = 10;
  static constexpr int64_t kUniqueSize = 1L << kUniqueBits;

  // 13 bits machine id
  static constexpr int kMachineBits = 13;
  static constexpr int64_t kMachineSize = 1L << kMachineBits;
  static constexpr int64_t kMachineBase = kMachineSize - 1024;  // use the last 1024 number

  // Configurations for virtual sharding
  static constexpr int kVirtualShardBits = 10;
  static constexpr int kVirtualShardCount = 1L << kVirtualShardBits;

  static constexpr int64_t kKafkaBackedSmyteIdStartMs = 1483228800000L;  // 2017-01-01

  // Generate a smyte id deterministically from kafka offset, timestamp and virtual shard.
  static SmyteId generateFromKafka(int64_t kafkaOffset, int64_t timestampMs, int virtualShard) {
    int64_t shiftedTimestamp = timestampMs - SmyteId::kTimestampEpoch;
    CHECK(shiftedTimestamp >= 0 && shiftedTimestamp < kTimestampSize)
        << "timestamp " << timestampMs << " for kafka offset " << kafkaOffset << " is out of range";
    CHECK(virtualShard >= 0 && virtualShard < kVirtualShardCount)
        << "virtual shard " << virtualShard << " for kafka offset " << kafkaOffset << " is out of range";
    int64_t unique = kafkaOffset % SmyteId::kUniqueSize;
    int64_t machine = SmyteId::kMachineBase + virtualShard;
    int64_t smyteId = (((shiftedTimestamp << SmyteId::kUniqueBits) + unique) << SmyteId::kMachineBits) + machine;

    return SmyteId(smyteId);
  }

  static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  explicit SmyteId(int64_t smyteId) : smyteId_(smyteId) {}

  explicit SmyteId(const std::string& smyteId)
      : smyteId_(boost::endian::detail::load_big_endian<int64_t, sizeof(int64_t)>(smyteId.data())) {}

  // Encode smyte id as an an 8-byte string. Use big endian so that it can be sorted in numerical order.
  std::string asBinary() const {
    std::string result;  // any reasonable compiler would apply Return Value Optimization here
    appendAsBinary(&result);
    return result;
  }

  // Append the smyte id to the output as an 8-byte string. Use big endian so that it can be sorted in numerical order.
  void appendAsBinary(std::string* out) const {
    boost::endian::big_int64_buf_t value(smyteId_);
    out->append(value.data(), sizeof(int64_t));
  }

  int getShardIndex(int shardCount) const {
    return (smyteId_ ^ (smyteId_ >> kMachineBits)) % shardCount;
  }

  // Return the virtual shard number [0, 1024) and -1 for error.
  int getVirtualShard() const {
    int64_t machine = smyteId_ % kMachineSize;
    if (machine < kMachineBase) return -1;
    return machine % kVirtualShardCount;
  }

  bool operator==(const SmyteId& rhs) const {
    return smyteId_ == rhs.smyteId_;
  }

  int64_t timestamp() const {
    return (smyteId_ >> (SmyteId::kUniqueBits +  SmyteId::kMachineBits)) + SmyteId::kTimestampEpoch;
  }

  int64_t machine() const {
    return smyteId_ & (kMachineSize - 1);
  }

  bool isGeneratedFromKafka() const {
    return timestamp() >= kKafkaBackedSmyteIdStartMs && machine() >= kMachineBase;
  }

  bool isValid() const {
    auto ts = timestamp();
    return ts >= kTimestampSaneEarliest && ts <= nowMs() + kTimestampSaneFutureMs;
  }

 private:
  int64_t smyteId_;
};

}  // namespace infra

#endif  // INFRA_SMYTEID_H_
