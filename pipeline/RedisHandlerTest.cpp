#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "pipeline/DatabaseManager.h"
#include "pipeline/RedisHandler.h"
#include "stesting/TestWithRocksDb.h"

namespace pipeline {

class RedisHandlerTest : public stesting::TestWithRocksDb {
 protected:
  RedisHandlerTest() {}
};

class MockRedisHandler : public RedisHandler {
 public:
  explicit MockRedisHandler(std::shared_ptr<DatabaseManager> databaseManager) : RedisHandler(databaseManager) {}

  MOCK_CONST_METHOD0(getCommandHandlerTable, const CommandHandlerTable&());

  // define protected functions from base class public in order to test them here
  bool validateArgCount(const std::vector<std::string>& cmd, size_t minArgs, size_t maxArgs) {
    return RedisHandler::validateArgCount(cmd, minArgs, maxArgs);
  }
};

TEST_F(RedisHandlerTest, ValidateArgCount) {
  MockRedisHandler handler(databaseManager());

  EXPECT_TRUE(handler.validateArgCount({"get", "a"}, 1, 1));
  EXPECT_TRUE(handler.validateArgCount({"ping"}, 0, 0));

  EXPECT_FALSE(handler.validateArgCount({"get", "a"}, 2, 2));
  EXPECT_FALSE(handler.validateArgCount({"ping"}, 1, 1));

  EXPECT_TRUE(handler.validateArgCount({"mget", "a"}, 1, 2));
  EXPECT_TRUE(handler.validateArgCount({"mget", "a", "b"}, 1, 2));
  EXPECT_TRUE(handler.validateArgCount({"mget", "a", "b"}, -1, 2));
  EXPECT_TRUE(handler.validateArgCount({"mget", "a"}, 1, -1));
  EXPECT_TRUE(handler.validateArgCount({"mget", "a", "b"}, 1, -1));
  EXPECT_TRUE(handler.validateArgCount({"mget", "a", "b", "c"}, 1, -1));

  EXPECT_FALSE(handler.validateArgCount({"mget"}, 1, 2));
  EXPECT_FALSE(handler.validateArgCount({"mget", "a", "b", "c"}, 1, 2));
  EXPECT_FALSE(handler.validateArgCount({"mget", "a", "b", "c"}, -1, 2));
  EXPECT_FALSE(handler.validateArgCount({"mget"}, 1, 0));

  EXPECT_FALSE(handler.validateArgCount({"mget"}, 2, 1));
}

}  // namespace pipeline
