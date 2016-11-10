#ifndef PIPELINE_REDISHANDLERBUILDER_H_
#define PIPELINE_REDISHANDLERBUILDER_H_

#include <memory>

#include "pipeline/RedisHandler.h"

namespace pipeline {

class RedisHandlerBuilder {
 public:
  virtual std::shared_ptr<RedisHandler> newHandler() = 0;
};

}  // namespace pipeline

#endif  // PIPELINE_REDISHANDLERBUILDER_H_
