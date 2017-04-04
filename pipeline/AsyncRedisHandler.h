#ifndef PIPELINE_ASYNCREDISHANDLER_H_
#define PIPELINE_ASYNCREDISHANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "boost/algorithm/string/case_conv.hpp"
#include "pipeline/RedisHandler.h"

namespace pipeline {

// A redis handler that allow command handlers to handle requests asynchronously while still guarantees that results
// are returned in the same order in which they were requested.
class AsyncRedisHandler : public RedisHandler {
 public:
  AsyncRedisHandler(std::shared_ptr<DatabaseManager> databaseManager,
                    std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper)
      : RedisHandler(databaseManager, consumerHelper) {}

  explicit AsyncRedisHandler(std::shared_ptr<DatabaseManager> databaseManager) : RedisHandler(databaseManager) {}

  // Allows the resulting redis pipeline to support async operations
  bool allowAsyncCommandHandler() const override {
    return true;
  }

  // Handler async commands by passing the request key to async command handlers
  bool handleCommand(int64_t key, const std::string& cmdNameLower, const std::vector<std::string>& cmd,
                     Context* ctx) override {
    auto handlerEntry = getAsyncCommandHandlerTable().find(cmdNameLower);
    if (handlerEntry == getAsyncCommandHandlerTable().end()) return false;

    if (!verifyCommandHandler(key, cmdNameLower, cmd, handlerEntry->second, ctx)) {
      // While verification failed, it is still an known command. Return true to information caller to stop searching.
      return true;
    }

    processCommandHandlerResult(key, (this->*(handlerEntry->second.handlerFunc))(key, cmd, ctx), ctx);
    return true;
  }

 protected:
  using AsyncCommandHandlerFunc = codec::RedisValue (AsyncRedisHandler::*)(int64_t key,
                                                                           const std::vector<std::string>& cmd,
                                                                           Context* ctx);
  using AsyncCommandHandlerTable = GenericCommandHandlerTable<AsyncCommandHandlerFunc>;

  // Command handlers inherited from the base redis handler
  static const CommandHandlerTable& baseCommandHandlerTable() {
    static const CommandHandlerTable commandHandlerTable(RedisHandler::mergeWithDefaultCommandHandlerTable({}));
    return commandHandlerTable;
  }

  const CommandHandlerTable& getCommandHandlerTable() const override {
    throw std::logic_error("Not supported by AsyncRedisHandler. Use getAsyncCommandHandlerTable");
  }

  virtual const AsyncCommandHandlerTable& getAsyncCommandHandlerTable() const = 0;

  // Merge client provided async command handler table with the default one
  static AsyncCommandHandlerTable mergeWithDefaultAsyncCommandHandlerTable(const AsyncCommandHandlerTable& newTable) {
    AsyncCommandHandlerTable baseTable;
    // Transform regular handlers to use async signature
    for (const auto& handlerEntry : baseCommandHandlerTable()) {
      baseTable.insert(
          {handlerEntry.first,
           {&AsyncRedisHandler::handleSyncCommand, handlerEntry.second.minArgs, handlerEntry.second.maxArgs}});
    }

    baseTable.insert(newTable.begin(), newTable.end());
    return baseTable;
  }

  codec::RedisValue handleSyncCommand(int64_t key, const std::vector<std::string>& cmd, Context* ctx) {
    auto handlerEntry = baseCommandHandlerTable().find(boost::to_lower_copy(cmd[0]));
    // Ignore the key for sync commands
    return (this->*(handlerEntry->second.handlerFunc))(cmd, ctx);
  }
};

}  // namespace pipeline

#endif  // PIPELINE_ASYNCREDISHANDLER_H_
