#ifndef CODEC_REDISENCODER_H_
#define CODEC_REDISENCODER_H_

#include <memory>

#include "codec/RedisValue.h"
#include "wangle/codec/MessageToByteEncoder.h"

namespace codec {

class RedisEncoder : public wangle::MessageToByteEncoder<RedisValue> {
 public:
  std::unique_ptr<folly::IOBuf> encode(RedisValue& msg) override {
    return folly::IOBuf::copyBuffer(msg.encode());
  }
};

}  // namespace codec

#endif  // CODEC_REDISENCODER_H_
