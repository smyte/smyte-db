#include "pipeline/TransactionalRedisHandler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "codec/RedisValue.h"
#include "glog/logging.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"

namespace pipeline {

bool TransactionalRedisHandler::handleCommandWithTransactionalHandlerTable(
    const std::string& cmdNameLower, const std::vector<std::string>& cmd,
    const TransactionalCommandHandlerTable& commandHandlerTable, Context* ctx) {
  // first check for MULTI/EXEC to determine transaction state transitions
  if (cmdNameLower == "multi") {
    if (inTransaction_) {
      // NOTE: nested MULTI is a error but it won't cancel the transaction
      write(ctx, codec::RedisValue(codec::RedisValue::Type::kError, "MULTI calls cannot be nested"));
    } else {
      inTransaction_ = true;
      write(ctx, simpleStringOk());
    }
  } else if (cmdNameLower == "exec") {
    if (inTransaction_) {
      if (errorEncountered_) {
        write(ctx, {codec::RedisValue::Type::kError, "Transaction discarded because of previous errors"});
      } else {
        std::vector<codec::RedisValue> results;
        rocksdb::WriteBatch writeBatch;
        for (const auto& cmd : queuedCommands_) {
          codec::RedisValue result = (this->*(cmd.first))(cmd.second, &writeBatch, ctx);
          if (result.type() == codec::RedisValue::Type::kError) {
            errorEncountered_ = true;
            break;
          }
          results.push_back(std::move(result));
        }
        if (errorEncountered_) {
          // NOTE: standard redis protocol does not abort the transaction for runtime errors
          write(ctx, {codec::RedisValue::Type::kError,
                      "Transaction discarded because an error was encountered during execution"});
        } else {
          writeResult(codec::RedisValue(std::move(results)), &writeBatch, ctx);
        }
      }
    } else {
      write(ctx, codec::RedisValue(codec::RedisValue::Type::kError, "EXEC without MULTI"));
    }
    resetTransactionState();
  } else {
    auto handlerEntry = commandHandlerTable.find(cmdNameLower);
    if (handlerEntry == commandHandlerTable.end()) {
      errorEncountered_ = true;
      return false;
    }

    if (!validateArgCount(cmd, handlerEntry->second.minArgs, handlerEntry->second.maxArgs)) {
      errorEncountered_ = true;
      writeError(folly::sformat(kWrongNumArgsTemplate, cmdNameLower), ctx);
      return true;
    }

    if (inTransaction_) {
      queuedCommands_.emplace_back(std::make_pair(handlerEntry->second.handlerFunc, std::move(cmd)));
      write(ctx, { codec::RedisValue::Type::kSimpleString, "QUEUED" });
    } else {
      // execute it right away when it's not part of the transaction
      rocksdb::WriteBatch writeBatch;
      writeResult((this->*(handlerEntry->second.handlerFunc))(cmd, &writeBatch, ctx), &writeBatch, ctx);
    }
  }

  return true;
}

void TransactionalRedisHandler::writeResult(codec::RedisValue result, rocksdb::WriteBatch* writeBatch, Context* ctx) {
  if (writeBatch->Count() > 0) {
    // commit updates first
    rocksdb::Status status = db()->Write(rocksdb::WriteOptions(), writeBatch);
    if (!status.ok()) {
      write(ctx, errorResp(folly::sformat("RocksDB error: {}", status.ToString())));
      return;
    }
  }
  // no updates or updates committed successfully, write the result back
  write(ctx, std::move(result));
}

}  // namespace pipeline
