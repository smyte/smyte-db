#include "codec/RedisValue.h"

#include <sstream>
#include <string>
#include <vector>

#include "boost/variant.hpp"
#include "glog/logging.h"

namespace codec {

std::string RedisValue::encode() const {
  std::stringstream ss;

  ss << RedisValue::kTypeIndicators[static_cast<int>(type())];
  switch (type()) {
  case Type::kInteger:
    ss << integer() << "\r\n";
    break;
  case Type::kError:
  case Type::kSimpleString:
  // fall through as error and simple string only differ in type indicator
  {
    // escape \r and \n
    const std::string& str = type() == Type::kError ? error() : simpleString();
    for (auto it = str.cbegin(); it != str.cend(); ++it) {
      if (*it == '\r') {
        ss << "\\r";
      } else if (*it == '\n') {
        ss << "\\n";
      } else {
        ss << *it;
      }
    }
    ss << "\r\n";
    break;
  }
  case Type::kBulkString:
  {
    const std::string& s = bulkString();
    ss << s.size() << "\r\n" << s << "\r\n";
    break;
  }
  case Type::kArray:
  {
    const std::vector<RedisValue>& elems = array();
    ss << elems.size() << "\r\n";
    for (const RedisValue& elem : elems) {
      ss << elem.encode();
    }
    break;
  }
  case Type::kBulkStringArray:
  {
    const std::vector<std::string>& elems = bulkStringArray();
    ss << elems.size() << "\r\n";
    for (const std::string& elem : elems) {
      ss << RedisValue::kTypeIndicators[static_cast<int>(Type::kBulkString)] << elem.size() << "\r\n"
         << elem << "\r\n";
    }
    break;
  }
  case Type::kNullString:
    ss << "-1\r\n";
    break;
  case Type::kAsyncResult:
    // pass through since it's not intended for encoding
  default:
    LOG(FATAL) << "Unknown RedisValue type: " << int(type());
    break;
  }

  return ss.str();
}

constexpr char RedisValue::kTypeIndicators[];

}  // namespace codec
