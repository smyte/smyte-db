#ifndef INFRA_SCHEDULEDTASK_H_
#define INFRA_SCHEDULEDTASK_H_

#include <string>
#include <utility>

#include "boost/endian/buffers.hpp"
#include "glog/logging.h"
#include "rocksdb/slice.h"

namespace infra {

class ScheduledTask {
 public:
  // Encode timestamp in big endian so that its numerical ordering is preserved by the lexicographical ordering of
  // the encoded string
  static rocksdb::Slice encodeTimestamp(int64_t timestampMs, std::string* buf) {
    // Valid timestamps should all be positive, but supporting zero here for special value encoding
    CHECK_GE(timestampMs, 0);
    boost::endian::big_int64_buf_t value(timestampMs);
    buf->assign(value.data(), sizeof(timestampMs));
    return *buf;
  }

  // Decode timestamp from a byte array of big endian
  static int64_t decodeTimestamp(const char* buf) {
    return boost::endian::detail::load_big_endian<int64_t, sizeof(int64_t)>(buf);
  }

  ScheduledTask(int64_t scheduledTimeMs, std::string dataKey, std::string value)
      : scheduledTimeMs_(scheduledTimeMs), dataKey_(std::move(dataKey)), value_(std::move(value)), completed_(false) {
    CHECK(!dataKey_.empty()) << "A ScheduledTask requires non-empty `dataKey`";
    encodeTimestamp(scheduledTimeMs, &key_);
    key_.append(dataKey_);
  }

  // Accessors functions
  const std::string& key() const {
    return key_;
  }
  int64_t scheduledTimeMs() const {
    return scheduledTimeMs_;
  }
  const std::string& dataKey() const {
    return dataKey_;
  }
  const std::string& value() const {
    return value_;
  }
  bool completed() const {
    return completed_;
  }

  // Signal that the task has been completed
  void markCompleted() {
    completed_ = true;
  }

  bool operator==(const ScheduledTask& rhs) const {
    // no need to check `key`, since it's derivative
    return scheduledTimeMs() == rhs.scheduledTimeMs() && dataKey() == rhs.dataKey() && value() == rhs.value();
  }

  bool operator!=(const ScheduledTask& rhs) const {
    return !(*this == rhs);
  }

 private:
  const int64_t scheduledTimeMs_;
  // Key for user-supplied data. It's necessary to distinguish between tasks scheduled at the same time.
  const std::string dataKey_;
  // Optional value for user-supplied data
  const std::string value_;
  // Whether this task has been completed
  bool completed_;
  // Key for the task itself, which is combination of scheduledTimeMs and dataKey
  std::string key_;
};

}  // namespace infra

#endif  // INFRA_SCHEDULEDTASK_H_
