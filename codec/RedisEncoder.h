#ifndef CODEC_REDISENCODER_H_
#define CODEC_REDISENCODER_H_

#include <memory>

#include "codec/RedisMessage.h"
#include "wangle/codec/MessageToByteEncoder.h"

namespace codec {

class RedisEncoder : public wangle::MessageToByteEncoder<RedisMessage> {
 public:
  std::unique_ptr<folly::IOBuf> encode(RedisMessage& msg) override {
    // Key in a redis message is only for internal use. There is no need for encoding.
    return folly::IOBuf::copyBuffer(msg.val.encode());
  }
};

}  // namespace codec

#endif  // CODEC_REDISENCODER_H_
