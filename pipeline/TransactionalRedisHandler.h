#ifndef PIPELINE_TRANSACTIONALREDISHANDLER_H_
#define PIPELINE_TRANSACTIONALREDISHANDLER_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/algorithm/string/case_conv.hpp"
#include "rocksdb/write_batch.h"
#include "pipeline/RedisHandler.h"

namespace pipeline {

// A redis handler that supports transactions
class TransactionalRedisHandler : public RedisHandler {
 public:
  explicit TransactionalRedisHandler(std::shared_ptr<DatabaseManager> databaseManager)
      : RedisHandler(databaseManager), inTransaction_(false), errorEncountered_(false) {}

  bool handleCommand(const std::string& cmdNameLower, const std::vector<std::string>& cmd, Context* ctx) override {
    return handleCommandWithTransactionalHandlerTable(cmdNameLower, cmd, getTransactionalCommandHandlerTable(), ctx);
  }

 protected:
  using TransactionalCommandHandlerFunc = codec::RedisValue (TransactionalRedisHandler::*)(
      const std::vector<std::string>& cmd, rocksdb::WriteBatch* writeBatch, Context* ctx);

  struct TransactionalCommandHandler {
    TransactionalCommandHandlerFunc handlerFunc = nullptr;
    int minArgs = 0;
    int maxArgs = 0;
    TransactionalCommandHandler(TransactionalCommandHandlerFunc _handlerFunc, int _minArgs, int _maxArgs)
        : handlerFunc(_handlerFunc), minArgs(_minArgs), maxArgs(_maxArgs) {}
  };
  using TransactionalCommandHandlerTable = std::unordered_map<std::string, TransactionalCommandHandler>;

  // Command handlers inherited from the base, non-transactional redis handler
  static const CommandHandlerTable& baseCommandHandlerTable() {
    static const CommandHandlerTable commandHandlerTable(RedisHandler::mergeWithDefaultCommandHandlerTable({}));
    return commandHandlerTable;
  }

  // Merge client provided transactional command handler table with the default one
  static TransactionalCommandHandlerTable mergeWithDefaultTransactionalCommandHandlerTable(
      const TransactionalCommandHandlerTable& newTable) {
    TransactionalCommandHandlerTable baseTable;
    // Transform non-transactional handlers to use transactional signature
    for (const auto& handlerEntry : baseCommandHandlerTable()) {
      baseTable.insert({handlerEntry.first,
                        {&TransactionalRedisHandler::handleNonTransactionalCommand, handlerEntry.second.minArgs,
                         handlerEntry.second.maxArgs}});
    }

    baseTable.insert(newTable.begin(), newTable.end());
    return baseTable;
  }

  bool handleCommandWithTransactionalHandlerTable(const std::string& cmdNameLower, const std::vector<std::string>& cmd,
                                                  const TransactionalCommandHandlerTable& commandHandlerTable,
                                                  Context* ctx);

  virtual const TransactionalCommandHandlerTable& getTransactionalCommandHandlerTable() const = 0;

  codec::RedisValue handleNonTransactionalCommand(const std::vector<std::string>& cmd, rocksdb::WriteBatch* writeBatch,
                                                  Context* ctx) {
    auto handlerEntry = baseCommandHandlerTable().find(boost::to_lower_copy(cmd[0]));
    return (this->*(handlerEntry->second.handlerFunc))(cmd, ctx);
  }

  const CommandHandlerTable& getCommandHandlerTable() const override {
    throw std::logic_error("Not supported by TransactionalCommandHandler");
  }

  void resetTransactionState() {
    inTransaction_ = false;
    errorEncountered_ = false;
    queuedCommands_.clear();
  }

 private:
  void writeResult(codec::RedisValue result, rocksdb::WriteBatch* writeBatch, Context* ctx);

  bool inTransaction_;
  bool errorEncountered_;
  // each command consists of a pair of TransactionalCommandHandlerFunc and a string vector
  std::vector<std::pair<TransactionalCommandHandlerFunc, std::vector<std::string>>> queuedCommands_;
};

}  // namespace pipeline

#endif  // PIPELINE_TRANSACTIONALREDISHANDLER_H_
