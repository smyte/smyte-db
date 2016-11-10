#include <string>
#include <vector>

#include "codec/RedisValue.h"
#include "gtest/gtest.h"

namespace codec {

TEST(RedisValueTest, DefaultConstructor) {
  RedisValue redisValue;
  EXPECT_EQ(RedisValue::Type::kNullString, redisValue.type());
  EXPECT_EQ(0, redisValue.integer());

  EXPECT_EQ("$-1\r\n", redisValue.encode());
}

TEST(RedisValueTest, Integer) {
  RedisValue redisValue(15);
  EXPECT_EQ(RedisValue::Type::kInteger, redisValue.type());
  EXPECT_EQ(15, redisValue.integer());

  EXPECT_EQ(":15\r\n", redisValue.encode());
}

TEST(RedisValueTest, Error) {
  RedisValue redisValue(RedisValue::Type::kError, "this is an error");
  EXPECT_EQ(RedisValue::Type::kError, redisValue.type());
  EXPECT_EQ("this is an error", redisValue.error());

  EXPECT_EQ("-this is an error\r\n", redisValue.encode());

  redisValue = RedisValue(RedisValue::Type::kError, "this is an\r\nerror");
  EXPECT_EQ(RedisValue::Type::kError, redisValue.type());
  EXPECT_EQ("this is an\r\nerror", redisValue.error());

  EXPECT_EQ("-this is an\\r\\nerror\r\n", redisValue.encode());
}

TEST(RedisValueTest, EmptyError) {
  RedisValue redisValue(RedisValue::Type::kError, "");
  EXPECT_EQ(RedisValue::Type::kError, redisValue.type());
  EXPECT_EQ("", redisValue.bulkString());

  EXPECT_EQ("-\r\n", redisValue.encode());
}

TEST(RedisValueTest, SimpleString) {
  RedisValue redisValue(RedisValue::Type::kSimpleString, "this is a simple string");
  EXPECT_EQ(RedisValue::Type::kSimpleString, redisValue.type());
  EXPECT_EQ("this is a simple string", redisValue.simpleString());

  EXPECT_EQ("+this is a simple string\r\n", redisValue.encode());

  redisValue = RedisValue(RedisValue::Type::kSimpleString, "this is a simple\r\nstring");
  EXPECT_EQ(RedisValue::Type::kSimpleString, redisValue.type());
  EXPECT_EQ("this is a simple\r\nstring", redisValue.simpleString());

  EXPECT_EQ("+this is a simple\\r\\nstring\r\n", redisValue.encode());
}

TEST(RedisValueTest, EmptySimpleString) {
  RedisValue redisValue(RedisValue::Type::kSimpleString, "");
  EXPECT_EQ(RedisValue::Type::kSimpleString, redisValue.type());
  EXPECT_EQ("", redisValue.bulkString());

  EXPECT_EQ("+\r\n", redisValue.encode());
}

TEST(RedisValueTest, BulkString) {
  RedisValue redisValue(RedisValue::Type::kBulkString, "this is a\r\nbulk string\r\n");
  EXPECT_EQ(RedisValue::Type::kBulkString, redisValue.type());
  EXPECT_EQ("this is a\r\nbulk string\r\n", redisValue.bulkString());

  EXPECT_EQ("$24\r\nthis is a\r\nbulk string\r\n\r\n", redisValue.encode());
}

TEST(RedisValueTest, EmptyBulkString) {
  RedisValue redisValue(RedisValue::Type::kBulkString, "");
  EXPECT_EQ(RedisValue::Type::kBulkString, redisValue.type());
  EXPECT_EQ("", redisValue.bulkString());

  EXPECT_EQ("$0\r\n\r\n", redisValue.encode());
}

TEST(RedisValueTest, Array) {
  RedisValue v1(1);
  RedisValue v2(2);
  RedisValue redisValue(std::vector<RedisValue>{v1, v2});
  EXPECT_EQ(RedisValue::Type::kArray, redisValue.type());
  EXPECT_EQ(2, redisValue.array().size());
  EXPECT_EQ(1, redisValue.array()[0].integer());
  EXPECT_EQ(2, redisValue.array()[1].integer());

  EXPECT_EQ("*2\r\n:1\r\n:2\r\n", redisValue.encode());
}

TEST(RedisValueTest, BulkStringArray) {
  RedisValue redisValue(std::vector<std::string>{"a\r\n1", "b\r\n2"});
  EXPECT_EQ(RedisValue::Type::kBulkStringArray, redisValue.type());
  EXPECT_EQ(2, redisValue.bulkStringArray().size());
  EXPECT_EQ("a\r\n1", redisValue.bulkStringArray()[0]);
  EXPECT_EQ("b\r\n2", redisValue.bulkStringArray()[1]);

  EXPECT_EQ("*2\r\n$4\r\na\r\n1\r\n$4\r\nb\r\n2\r\n", redisValue.encode());
}

TEST(RedisValueTest, NullString) {
  EXPECT_EQ("$-1\r\n", RedisValue::nullString().encode());
}

TEST(RedisValueTest, EmptyListOrSet) {
  EXPECT_EQ("*0\r\n", RedisValue::emptyListOrSet().encode());
}

TEST(RedisValueTest, ArrayOfArrays) {
  RedisValue v1(1);
  RedisValue v2(2);
  RedisValue integersRedisValue(std::vector<RedisValue>{v1, v2});
  RedisValue stringsRedisValue(std::vector<std::string>{"a\r\n1", "b\r\n2"});
  RedisValue arrayRedisValue(std::vector<RedisValue>{integersRedisValue, stringsRedisValue});

  EXPECT_EQ(RedisValue::Type::kArray, arrayRedisValue.type());
  EXPECT_EQ(2, arrayRedisValue.array().size());
  EXPECT_EQ(2, arrayRedisValue.array()[0].array().size());
  EXPECT_EQ(2, arrayRedisValue.array()[1].bulkStringArray().size());
  EXPECT_EQ(1, arrayRedisValue.array()[0].array()[0].integer());
  EXPECT_EQ(2, arrayRedisValue.array()[0].array()[1].integer());
  EXPECT_EQ("a\r\n1", arrayRedisValue.array()[1].bulkStringArray()[0]);
  EXPECT_EQ("b\r\n2", arrayRedisValue.array()[1].bulkStringArray()[1]);

  EXPECT_EQ("*2\r\n*2\r\n:1\r\n:2\r\n*2\r\n$4\r\na\r\n1\r\n$4\r\nb\r\n2\r\n", arrayRedisValue.encode());
}

TEST(RedisValueTest, EmptyArray) {
  RedisValue redisValue(std::vector<RedisValue>{});
  EXPECT_EQ(RedisValue::Type::kArray, redisValue.type());
  EXPECT_TRUE(redisValue.array().empty());

  EXPECT_EQ("*0\r\n", redisValue.encode());
}

}  // namespace codec
