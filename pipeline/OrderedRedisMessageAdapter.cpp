#include "pipeline/OrderedRedisMessageAdapter.h"

namespace pipeline {

folly::Future<folly::Unit> OrderedRedisMessageAdapter::write(Context* ctx, codec::RedisMessage msg) {
  if (msg.key == -1) {
    // this is a special message not corresponds to any request
    return ctx->fireWrite(std::move(msg));
  }

  // Make sure key is valid
  size_t index = msg.key - startKey_;
  CHECK(index >= 0 && index < pendingOutputs_.size());
  auto& pendingOutput = pendingOutputs_[index];
  CHECK_EQ(pendingOutput.key, msg.key);

  // Make sure val is valid and the specified pending output has not been updated before
  CHECK_EQ(pendingOutput.val.type(), codec::RedisValue::Type::kAsyncResult);
  CHECK_NE(msg.val.type(), codec::RedisValue::Type::kAsyncResult);

  // Assign the value to corresponding entry but only fire the writes in order
  pendingOutput.val = std::move(msg.val);
  auto future = folly::makeFuture();
  for (auto it = pendingOutputs_.begin(); it != pendingOutputs_.end();) {
    if (it->val.type() == codec::RedisValue::Type::kAsyncResult) {
      // Encountered a pending output that has not been fulfilled yet, stop writing
      return std::move(future);
    }
    future = ctx->fireWrite(*it);
    it = pendingOutputs_.erase(it);
    startKey_++;
  }

  return future;
}

}  // namespace pipeline
