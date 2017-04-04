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

#include "codec/RedisMessage.h"
#include "folly/Conv.h"
#include "folly/SocketAddress.h"
#include "glog/logging.h"
#include "infra/kafka/ConsumerHelper.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"
#include "pipeline/DatabaseManager.h"
#include "wangle/channel/Handler.h"

namespace pipeline {

class RedisHandler : public wangle::HandlerAdapter<codec::RedisMessage> {
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

  // DatabaseManager is required while ConsumerHelper is optional
  RedisHandler(std::shared_ptr<DatabaseManager> databaseManager,
               std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper)
      : databaseManager_(databaseManager), consumerHelper_(consumerHelper) {}

  explicit RedisHandler(std::shared_ptr<DatabaseManager> databaseManager)
      : RedisHandler(databaseManager, nullptr) {}

  virtual ~RedisHandler() {}

  void read(Context* ctx, codec::RedisMessage req) override;

  void readEOF(Context* ctx) override { close(ctx); }
  void readException(Context* ctx, folly::exception_wrapper e) override { close(ctx); }

  folly::Future<folly::Unit> close(Context* ctx) override {
    DLOG(INFO) << "Connection closing";
    // use -1 as a special key to indicate that go away message is not specific to any request
    write(ctx, codec::RedisMessage(-1, codec::RedisValue::goAway()));
    removeMonitor(ctx);
    connectionClosed();
    return ctx->fireClose();
  }

  // Handle a Redis command.
  // @cmd contains both the command name (cmd[0]) and optionally, the arguments (cmd[1..])
  // Return true if the command is handled by the method; false otherwise.
  // Most clients should be okay with this default implementation and only need to override getCommandHandlerTable
  // to define its own handler functions. Sophisticated clients may override this method directly.
  virtual bool handleCommand(int64_t key, const std::string& cmdNameLower, const std::vector<std::string>& cmd,
                             Context* ctx) {
    auto handlerEntry = getCommandHandlerTable().find(cmdNameLower);
    if (handlerEntry == getCommandHandlerTable().end()) return false;

    if (verifyCommandHandler(key, cmdNameLower, cmd, handlerEntry->second, ctx)) {
      processCommandHandlerResult(key, (this->*(handlerEntry->second.handlerFunc))(cmd, ctx), ctx);
    }

    // Verification may have failed, but it is a known command regardless. Return true to ask caller to stop searching.
    return true;
  }

  // Specify whether this redis handler supports async commands.
  // An async command handler can respond to redis requests asynchronously while maintaining the correct order
  // when returning results to the clients. If true, this feature carries a small overhead in I/O threads.
  virtual bool allowAsyncCommandHandler() const {
    return false;
  }

 protected:
  using CommandHandlerFunc = codec::RedisValue (RedisHandler::*)(const std::vector<std::string>& cmd, Context* ctx);
  template <typename FuncType>
  struct CommandHandler {
    FuncType handlerFunc = nullptr;
    int minArgs = 0;
    int maxArgs = 0;
    CommandHandler(FuncType _handlerFunc, int _minArgs, int _maxArgs)
        : handlerFunc(_handlerFunc), minArgs(_minArgs), maxArgs(_maxArgs) {}
  };
  template <typename CommandHandlerFuncType>
  using GenericCommandHandlerTable = std::unordered_map<std::string, CommandHandler<CommandHandlerFuncType>>;
  // Default CommandHandlerTable type
  using CommandHandlerTable = GenericCommandHandlerTable<CommandHandlerFunc>;

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
      { "ready", { &RedisHandler::readyCommand, 0, 0 } },
      { "setready", { &RedisHandler::setReadyCommand, 0, 0 } },
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

  // Verify command handler function. Currently it only checks argument count.
  template <typename CommandHandlerFuncType>
  bool verifyCommandHandler(int64_t key, const std::string& cmdNameLower, const std::vector<std::string>& cmd,
                            const CommandHandler<CommandHandlerFuncType>& commandHandler, Context* ctx) {
    if (!validateArgCount(cmd, commandHandler.minArgs, commandHandler.maxArgs)) {
      writeError(key, folly::sformat(kWrongNumArgsTemplate, cmdNameLower), ctx);
      return false;
    }

    return true;
  }

  // Process the result returned from command handler function.
  virtual void processCommandHandlerResult(int64_t key, codec::RedisValue&& result, Context* ctx) {
    // A sync command writes result directly. An async command may do so at a later time.
    if (result.type() == codec::RedisValue::Type::kAsyncResult) {
      CHECK(allowAsyncCommandHandler()) << "Use AsyncRedisHandler for redis commands producing async results";
    } else {
      write(ctx, codec::RedisMessage(key, std::move(result)));
    }
  }

  // Provide a table handler functions, which allow clients to customize their command handling
  virtual const CommandHandlerTable& getCommandHandlerTable() const = 0;

  rocksdb::DB* db() const { return databaseManager_->db(); }
  std::shared_ptr<DatabaseManager> databaseManager() const { return databaseManager_; }

  codec::RedisValue errorResp(std::string&& msg) {
    LOG(ERROR) << "Error sent to client: " << msg;
    return codec::RedisValue(codec::RedisValue::Type::kError, std::move(msg));
  }

  void writeError(int64_t key, std::string&& msg, Context* ctx) {
    LOG(ERROR) << "Error sent to client: " << msg;
    write(ctx, codec::RedisMessage(key, {codec::RedisValue::Type::kError, std::move(msg)}));
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
  codec::RedisValue readyCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue setReadyCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue selectCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue setMetaCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue sleepCommand(const std::vector<std::string>& cmd, Context* ctx);
  codec::RedisValue thawCommand(const std::vector<std::string>& cmd, Context* ctx);

  void broadcastCmd(const std::vector<std::string>& cmd, Context* ctx);
  void outputStatistics(const std::string& name, const rocksdb::HistogramData& histData, std::stringstream* ss);
  void removeMonitor(Context* ctx);
  void writeToMonitorContext(const std::vector<std::string>& cmd, const std::string& monitorAddr, Context* ctx);

  std::shared_ptr<DatabaseManager> databaseManager_;
  std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper_;
};

}  // namespace pipeline

#endif  // PIPELINE_REDISHANDLER_H_
