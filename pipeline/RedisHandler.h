#ifndef PIPELINE_REDISHANDLER_H_
#define PIPELINE_REDISHANDLER_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

#include "codec/RedisValue.h"
#include "folly/Conv.h"
#include "folly/SocketAddress.h"
#include "glog/logging.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"
#include "pipeline/DatabaseManager.h"
#include "wangle/channel/Handler.h"

namespace pipeline {

class RedisHandler : public wangle::HandlerAdapter<codec::RedisValue> {
 public:
  // A RedisValue eventually gets copied to pipeline's write function
  // So so use a copy-friendly return value instead of const reference to static member
  static codec::RedisValue errorInvalidInteger() {
    return { codec::RedisValue::Type::kError, "Value is not an integer or out of range" };
  }
  static codec::RedisValue errorSyntaxError() {
    return { codec::RedisValue::Type::kError, "Syntax error" };
  }
  static codec::RedisValue errorNotRedisArray() {
    return { codec::RedisValue::Type::kError, "Not a Redis Array of Bulk String" };
  }
  static codec::RedisValue internalServerError() {
    return { codec::RedisValue::Type::kError, "Internal server error" };
  }
  static codec::RedisValue simpleStringOk() {
    return { codec::RedisValue::Type::kSimpleString, "OK" };
  }

  static bool parseInt(const std::string& value, int64_t* intValue) {
    try {
      *intValue = folly::to<int64_t>(value);
      return true;
    } catch (folly::ConversionError&) {
      return false;
    }
  }

  static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  static void connectionOpened() { connectionCount_++; }
  static void connectionClosed() { connectionCount_--; }
  static size_t getConnectionCount() { return connectionCount_; }

  explicit RedisHandler(std::shared_ptr<DatabaseManager> databaseManager)
      : databaseManager_(databaseManager) {}
  virtual ~RedisHandler() {}

  void read(Context* ctx, codec::RedisValue req) override;

  void readEOF(Context* ctx) override { close(ctx); }
  void readException(Context* ctx, folly::exception_wrapper e) override { close(ctx); }

  folly::Future<folly::Unit> close(Context* ctx) override {
    DLOG(INFO) << "Connection closing";
    write(ctx, codec::RedisValue::goAway());
    removeMonitor(ctx);
    connectionClosed();
    return ctx->fireClose();
  }

  // Handle a Redis command.
  // @cmd contains both the command name (cmd[0]) and optionally, the arguments (cmd[1..])
  // Return true if the command is handled by the method; false otherwise
  virtual bool handleCommand(const std::string& cmdNameLower, const std::vector<std::string>& cmd, Context* ctx) {
    return handleCommandWithHandlerTable(cmdNameLower, cmd, getCommandHandlerTable(), ctx);
  }

 protected:
  using CommandHandlerFunc = codec::RedisValue (RedisHandler::*)(const std::vector<std::string>& cmd, Context* ctx);
  struct CommandHandler {
    CommandHandlerFunc handlerFunc = nullptr;
    int minArgs = 0;
    int maxArgs = 0;
    CommandHandler(CommandHandlerFunc _handlerFunc, int _minArgs, int _maxArgs)
        : handlerFunc(_handlerFunc), minArgs(_minArgs), maxArgs(_maxArgs) {}
  };
  using CommandHandlerTable = std::unordered_map<std::string, CommandHandler>;

  static constexpr char kWrongNumArgsTemplate[] = "Wrong number of arguments for '{}' command";

  // Merge client provided command handler table with the default one
  static CommandHandlerTable mergeWithDefaultCommandHandlerTable(const CommandHandlerTable& newTable) {
    CommandHandlerTable baseTable({
      // default command handlers
      { "compact", { &RedisHandler::compactCommand, 0, 3 } },
      { "freeze", { &RedisHandler::freezeCommand, 0, 0 } },
      { "getmeta", { &RedisHandler::getMetaCommand, 1, 1 } },
      { "info", { &RedisHandler::infoCommand, 0, 1 } },
      { "monitor", { &RedisHandler::monitorCommand, 0, 0 } },
      { "ping", { &RedisHandler::pingCommand, 0, 0 } },
      { "select", { &RedisHandler::selectCommand, 1, 1 } },
      { "setmeta", { &RedisHandler::setMetaCommand, 2, 2 } },
      { "sleep", { &RedisHandler::sleepCommand, 1, 1 } },
      { "thaw", { &RedisHandler::thawCommand, 0, 0 } },
    });
    baseTable.insert(newTable.begin(), newTable.end());
    return baseTable;
  }

  static std::string getPeerAddressPortStr(Context* ctx) {
    try {
      folly::SocketAddress peerAddress;
      ctx->getTransport()->getPeerAddress(&peerAddress);
      return peerAddress.describe();
    } catch (...) {
      return "unknown address";
    }
  }

  // Convenience method that takes a command handler table with function pointers to handle redis commands
  // Most clients should be okay with this default implementation and only need to override getCommandHandlerTable
  // to define its own handler functions
  virtual bool handleCommandWithHandlerTable(const std::string& cmdNameLower, const std::vector<std::string>& cmd,
                                             const CommandHandlerTable& commandHandlerTable, Context* ctx) {
    auto handlerEntry = commandHandlerTable.find(cmdNameLower);
    if (handlerEntry == commandHandlerTable.end()) return false;

    if (validateArgCount(cmd, handlerEntry->second.minArgs, handlerEntry->second.maxArgs)) {
      auto result = (this->*(handlerEntry->second.handlerFunc))(cmd, ctx);
      // A sync command writes result directly. An async command may do so at a later time.
      if (result.type() != codec::RedisValue::Type::kAsyncResult) {
        write(ctx, std::move(result));
      }
    } else {
      writeError(folly::sformat(kWrongNumArgsTemplate, cmdNameLower), ctx);
    }

    return true;
  }

  // Provide a table handler functions, which allow clients to customize their command handling
  virtual const CommandHandlerTable& getCommandHandlerTable() const = 0;

  rocksdb::DB* db() const { return databaseManager_->db(); }
  std::shared_ptr<DatabaseManager> databaseManager() const { return databaseManager_; }

  codec::RedisValue errorResp(std::string&& msg) {
    LOG(ERROR) << "Error sent to client: " << msg;
    return codec::RedisValue(codec::RedisValue::Type::kError, std::move(msg));
  }

  void writeError(std::string&& msg, Context* ctx) {
    LOG(ERROR) << "Error sent to client: " << msg;
    write(ctx, codec::RedisValue(codec::RedisValue::Type::kError, std::move(msg)));
  }

  // Allow subclasses to customize the output of info command
  virtual void appendToInfoOutput(std::stringstream* ss);

  // check if the arguments of a command is within the given range [minArgs, maxArgs] (both bounds are inclusive)
  // -1 indicates to skip the boundary check
  bool validateArgCount(const std::vector<std::string>& cmd, int minArgs, int maxArgs);

 private:
  static std::vector<Context*> monitors_;
  static std::mutex monitorMutex_;
  static std::atomic<size_t> connectionCount_;

  codec::RedisValue compactCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue freezeCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue getMetaCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue infoCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue monitorCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue pingCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue selectCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue setMetaCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue sleepCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue thawCommand(const std::vector<std::string>& cmd, Context* ctx);

  void broadcastCmd(const std::vector<std::string>& cmd, Context* ctx);
  void outputStatistics(const std::string& name, const rocksdb::HistogramData& histData, std::stringstream* ss);
  void removeMonitor(Context* ctx);
  void writeToMonitorContext(const std::vector<std::string>& cmd, const std::string& monitorAddr, Context* ctx);

  std::shared_ptr<DatabaseManager> databaseManager_;
};

}  // namespace pipeline

#endif  // PIPELINE_REDISHANDLER_H_
