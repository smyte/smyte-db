#ifndef CODEC_REDISDECODER_H_
#define CODEC_REDISDECODER_H_

#include "wangle/codec/ByteToMessageDecoder.h"

#include "codec/RedisMessage.h"

namespace codec {

// A redis protocol decoder. Because it is only used for clients sending requests to servers, decoding Array of Bulk
// String is sufficient.
//
// For example, a PING request is encoded as follows:
// *1\r\n$4\r\nping\r\n
// The goal of this decoder is parse such request into a RedisValue wrapped in a RedisMessage with default key.
class RedisDecoder : public wangle::ByteToMessageDecoder<RedisMessage> {
 public:
  bool decode(Context* ctx, folly::IOBufQueue& buf, RedisMessage& result, size_t& needed) override;

 private:
  enum class LengthFieldState {
    kInvalid,
    kMoreBytesNeeded,
    kValid,
  };
  static constexpr size_t kMinBytesNeeded = 2;  // '\r\n'
  int64_t readLength(char typeIndicator, folly::io::Cursor* c, LengthFieldState* state, size_t* needed);
  void skipNoise(folly::io::Cursor* c);
};

}  // namespace codec

#endif  // CODEC_REDISDECODER_H_
