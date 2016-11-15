#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "codec/RedisValue.h"
#include "folly/Format.h"
#include "glog/logging.h"
#include "pipeline/RedisHandler.h"
#include "pipeline/RedisPipelineBootstrap.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

namespace key_value {

// A simple key-value store that uses redis protocol and persists data in rocksdb
class KeyValueHandler: public pipeline::RedisHandler {
 public:
  explicit KeyValueHandler(std::shared_ptr<pipeline::DatabaseManager> databaseManager)
      : RedisHandler(databaseManager) {}

  const CommandHandlerTable& getCommandHandlerTable() const override {
    static const CommandHandlerTable commandHandlerTable(mergeWithDefaultCommandHandlerTable({
        {"get", {static_cast<CommandHandlerFunc>(&KeyValueHandler::getCommand), 1, 1}},  // requires 1 parameter
        {"set", {static_cast<CommandHandlerFunc>(&KeyValueHandler::setCommand), 2, 2}},  // requires 2 parameters
    }));
    return commandHandlerTable;
  }

 private:
  codec::RedisValue getCommand(const std::vector<std::string>& cmd, Context* ctx) {
    rocksdb::Slice key = rocksdb::Slice(cmd[1]);

    std::string value;
    rocksdb::Status status = db()->Get(rocksdb::ReadOptions(), key, &value);

    if (status.ok()) {
      return codec::RedisValue(codec::RedisValue::Type::kBulkString, std::move(value));
    }

    if (!status.IsNotFound()) {
      return errorResp(folly::sformat("RocksDB error: {}", status.ToString()));
    }

    return codec::RedisValue::nullString();
  }

  codec::RedisValue setCommand(const std::vector<std::string>& cmd, Context* ctx) {
    rocksdb::Slice key = rocksdb::Slice(cmd[1]);
    rocksdb::Status status = db()->Put(rocksdb::WriteOptions(), key, cmd[2]);

    if (status.ok()) {
      return simpleStringOk();
    }

    return errorResp(folly::sformat("RocksDB error: {}", status.ToString()));
  }
};

static pipeline::RedisPipelineBootstrap::Config config{
  redisHandlerFactory : [](const pipeline::RedisPipelineBootstrap::OptionalComponents& optionalComponents) {
    std::shared_ptr<pipeline::RedisHandler> handler =
        std::make_shared<KeyValueHandler>(optionalComponents.databaseManager);
    return handler;
  },
};

static auto redisPipelineBootstrap = pipeline::RedisPipelineBootstrap::create(config);

}  // namespace key_value
