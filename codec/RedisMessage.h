#ifndef CODEC_REDISMESSAGE_H_
#define CODEC_REDISMESSAGE_H_

#include <utility>

#include "codec/RedisValue.h"

namespace codec {

struct RedisMessage {
  RedisMessage() : RedisMessage(0, RedisValue()) {}

  explicit RedisMessage(codec::RedisValue&& _val) : RedisMessage(0, std::move(_val)) {}

  RedisMessage(int64_t _key, codec::RedisValue&& _val) : key(_key), val(std::move(_val)) {}

  bool operator==(const RedisMessage& rhs) const {
    return key == rhs.key && val == rhs.val;
  }

  int64_t key;
  codec::RedisValue val;
};

}  // namespace codec

#endif  // CODEC_REDISMESSAGE_H_
