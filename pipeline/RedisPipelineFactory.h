#ifndef PIPELINE_REDISPIPELINEFACTORY_H_
#define PIPELINE_REDISPIPELINEFACTORY_H_

#include <memory>
#include <utility>

#include "codec/RedisDecoder.h"
#include "codec/RedisEncoder.h"
#include "codec/RedisMessage.h"
#include "folly/io/IOBufQueue.h"
#include "pipeline/OrderedRedisMessageAdapter.h"
#include "pipeline/RedisHandler.h"
#include "pipeline/RedisHandlerBuilder.h"
#include "wangle/channel/AsyncSocketHandler.h"
#include "wangle/channel/OutputBufferingHandler.h"
#include "wangle/channel/Pipeline.h"

namespace pipeline {

using RedisPipeline = wangle::Pipeline<folly::IOBufQueue&, codec::RedisMessage>;

class RedisPipelineFactory : public wangle::PipelineFactory<RedisPipeline> {
 public:
  explicit RedisPipelineFactory(std::shared_ptr<RedisHandlerBuilder> redisHandlerBuilder)
      : redisDecoder_(std::make_shared<codec::RedisDecoder>()),
        redisEncoder_(std::make_shared<codec::RedisEncoder>()),
        redisHandlerBuilder_(redisHandlerBuilder) {}

  RedisPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) override {
    auto pipeline = RedisPipeline::create();
    pipeline->addBack(wangle::AsyncSocketHandler(sock));
    pipeline->addBack(wangle::OutputBufferingHandler());
    pipeline->addBack(redisDecoder_);
    pipeline->addBack(redisEncoder_);
    auto redisHandler = redisHandlerBuilder_->newHandler();
    if (redisHandler->allowAsyncCommandHandler()) {
      pipeline->addBack(std::make_shared<OrderedRedisMessageAdapter>());
    }
    pipeline->addBack(std::move(redisHandler));
    pipeline->finalize();
    return pipeline;
  }

 private:
  std::shared_ptr<codec::RedisDecoder> redisDecoder_;
  std::shared_ptr<codec::RedisEncoder> redisEncoder_;
  std::shared_ptr<RedisHandlerBuilder> redisHandlerBuilder_;
};

}  // namespace pipeline

#endif  // PIPELINE_REDISPIPELINEFACTORY_H_
