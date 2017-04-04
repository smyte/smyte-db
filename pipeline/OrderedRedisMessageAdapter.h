#ifndef PIPELINE_ORDEREDREDISMESSAGEADAPTER_H_
#define PIPELINE_ORDEREDREDISMESSAGEADAPTER_H_

#include <deque>
#include <utility>

#include "codec/RedisMessage.h"
#include "wangle/channel/Handler.h"

namespace pipeline {

// Generate redis message keys based on their receiving order and write the corresponding output in the same order
class OrderedRedisMessageAdapter : public wangle::HandlerAdapter<codec::RedisMessage> {
 public:
  OrderedRedisMessageAdapter() : startKey_(0), pendingOutputs_() {}

  void read(Context* ctx, codec::RedisMessage msg) override {
    // Current key increases monotonically as client requests arrive
    msg.key = startKey_ + pendingOutputs_.size();
    // async result represents requests that have not been fulfilled
    pendingOutputs_.push_back(codec::RedisMessage(msg.key, codec::RedisValue::asyncResult()));
    ctx->fireRead(std::move(msg));
  }

  folly::Future<folly::Unit> write(Context* ctx, codec::RedisMessage msg) override;

 private:
  int64_t startKey_;
  std::deque<codec::RedisMessage> pendingOutputs_;
};

}  // namespace pipeline

#endif  // PIPELINE_ORDEREDREDISMESSAGEADAPTER_H_
