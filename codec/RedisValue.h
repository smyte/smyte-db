#ifndef CODEC_REDISVALUE_H_
#define CODEC_REDISVALUE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/endian/buffers.hpp"
#include "boost/variant.hpp"
#include "glog/logging.h"

namespace codec {

class RedisValue {
 public:
  using IntType = int64_t;
  using DataType = boost::variant<IntType, std::string, std::vector<RedisValue>, std::vector<std::string>>;
  enum class Type {
    kInteger,
    kError,
    kSimpleString,
    kBulkString,
    kArray,
    // special types
    kBulkStringArray,  // it's a common case for kArray
    kNullString,
    kAsyncResult,  // it's a special type to indicate that the actual result will be generated asynchronously
  };
  static constexpr char kTypeIndicators[] = {
    ':',  // kInteger
    '-',  // kError
    '+',  // kSimpleString
    '$',  // kBulkString
    '*',  // kArray
    '*',  // kBulkStringArray
    '$',  // kNullString
  };

  static RedisValue nullString() {
    return RedisValue();
  }

  static RedisValue goAway() {
    return RedisValue(codec::RedisValue::Type::kError, "GOAWAY");
  }

  static RedisValue emptyListOrSet() {
    return RedisValue(std::vector<RedisValue>{});
  }

  static RedisValue asyncResult() {
    return RedisValue(Type::kAsyncResult, "");
  }

  inline static RedisValue fromLong(int64_t longValue) {
    boost::endian::big_int64_buf_t value(longValue);
    return RedisValue(Type::kBulkString, std::string(value.data(), sizeof(int64_t)));
  }

  static RedisValue smyteIdBinary(int64_t smyteId) {
    CHECK(smyteId > 0) << "SmyteId value not in expected range";
    return fromLong(smyteId);
  }

  RedisValue() : type_(Type::kNullString), data_(0) {}
  explicit RedisValue(IntType data) : type_(Type::kInteger), data_(data) {}
  // The following constructors take rvalue-typed parameters explicitly, which allows compiler to identify places
  // where we should apply std::move to reduce copy operations.
  // The down side of explicit rvalue signature, when compared to using perfect forwarding, is that the caller will
  // have to make an explicit copy when move is not possible. Because RedisValue type is used extensively throughout
  // the code base, making move/copy decisions explicitly helps us write the most efficient code possible.
  explicit RedisValue(std::vector<RedisValue>&& data) : type_(Type::kArray), data_(std::move(data)) {}
  explicit RedisValue(std::vector<std::string>&& data) : type_(Type::kBulkStringArray), data_(std::move(data)) {}
  RedisValue(Type type, std::string&& data) : type_(type), data_(std::move(data)) {}

  Type type() const { return type_; }
  IntType integer() const { return boost::get<IntType>(data_); }
  const std::string& error() const { return boost::get<std::string>(data_); }
  const std::string& simpleString() const { return boost::get<std::string>(data_); }
  const std::string& bulkString() const { return boost::get<std::string>(data_); }
  const std::vector<RedisValue>& array() const { return boost::get<std::vector<RedisValue>>(data_); }
  const std::vector<std::string>& bulkStringArray() const { return boost::get<std::vector<std::string>>(data_); }

  std::string encode() const;

  bool operator==(const RedisValue& rhs) const {
    if (type() != rhs.type()) return false;

    if (type() == Type::kArray) {
      auto lhsArray = array();
      auto rhsArray = rhs.array();
      if (lhsArray.size() != rhsArray.size()) return false;

      for (size_t i = 0; i < lhsArray.size(); i++) {
        if (lhsArray[i] != rhsArray[i]) return false;
      }

      return true;
    } else if (type() == Type::kBulkStringArray) {
      auto lhsArray = bulkStringArray();
      auto rhsArray = rhs.bulkStringArray();
      if (lhsArray.size() != rhsArray.size()) return false;

      for (size_t i = 0; i < lhsArray.size(); i++) {
        if (lhsArray[i] != rhsArray[i]) return false;
      }

      return true;
    } else {
      return data_ == rhs.data_;
    }
  }

  bool operator!=(const RedisValue& rhs) const {
    return !operator==(rhs);
  }

 private:
  Type type_;
  DataType data_;
};

}  // namespace codec

#endif  // CODEC_REDISVALUE_H_
