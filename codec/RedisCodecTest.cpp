#include <string>
#include <vector>

#include "codec/RedisDecoder.h"
#include "codec/RedisEncoder.h"
#include "codec/RedisValue.h"
#include "folly/io/IOBuf.h"
#include "gtest/gtest.h"

namespace codec {

TEST(RedisDecoder, Incomplete) {
  RedisDecoder decoder;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  RedisValue result;
  size_t needed = 0;
  std::string input;

  // return false for incomplete input and IOBufQueue is not trimmed

  input = "\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());

  input = "\r\n\r";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(1, needed);
  EXPECT_EQ(1, queue.chainLength());

  input = "\r\n\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());

  input = "\r\n\r\n\r";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(1, needed);
  EXPECT_EQ(1, queue.chainLength());

  input = "\r\n\r\n*3";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);
  EXPECT_EQ(2, queue.chainLength());

  input = "***2";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*1234";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(1, needed);  // '\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nge";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(3, needed);  // 't\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget\r";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(1, needed);  // '\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n$2\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(4, needed);  // 'ab\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n$2\r\na";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(3, needed);  // 'b\r\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n$2\r\nab\r";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(1, needed);  // '\n'
  EXPECT_EQ(input.size(), queue.chainLength());

  // data spreads across two buffers
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer("\r"));
  queue.append(folly::IOBuf::copyBuffer("\n\r\n"));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());

  // chain start with empty buffer
  queue.pop_front();
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(""));
  queue.append(folly::IOBuf::copyBuffer("\r\n\r\n"));
  needed = 0;
  EXPECT_FALSE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());
}

TEST(RedisDecoder, Invalid) {
  RedisDecoder decoder;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  RedisValue result;
  size_t needed = 0;
  std::string input;

  // returns true with a valid RedisValue that represents error and IOBufQueue trimmed

  input = "*\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());

  input = "*a\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\t";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\n$\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(2, needed);
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\n$a\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\n$1\r\t";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*0\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*-1\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*-1\r\n*3";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Array length\r\n", result.encode());
  EXPECT_EQ(0, needed);
  EXPECT_EQ(2, queue.chainLength());

  input = "*1\r\n$0\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(2, needed);   // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\n$0\r\n\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(0, needed);
  EXPECT_EQ(2, queue.chainLength());

  input = "*1\r\n$-1\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*1\r\n$-1\r\n*3";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("-Protocol Error: Invalid Bulk String length\r\n", result.encode());
  EXPECT_EQ(0, needed);
  EXPECT_EQ(2, queue.chainLength());
}

TEST(RedisDecoder, Valid) {
  RedisDecoder decoder;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  RedisValue result;
  size_t needed = 0;
  std::string input;

  // returns true with a non-error RedisValue and IOBufQueue trimmed

  input = "*2\r\n$3\r\nget\r\n$2\r\nab\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ(input, result.encode());
  EXPECT_EQ(2, needed);  // '\r\n'
  EXPECT_EQ(0, queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n$2\r\nab\r\n*3";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("*2\r\n$3\r\nget\r\n$2\r\nab\r\n", result.encode());
  EXPECT_EQ(0, needed);
  EXPECT_EQ(2, queue.chainLength());

  input = "*2\r\n$3\r\nget\r\n$2\r\nab\r\n*3\r\n";
  queue.pop_front();
  queue.clear();
  queue.append(folly::IOBuf::copyBuffer(input));
  needed = 0;
  EXPECT_TRUE(decoder.decode(nullptr, queue, result, needed));
  EXPECT_EQ("*2\r\n$3\r\nget\r\n$2\r\nab\r\n", result.encode());
  EXPECT_EQ(0, needed);
  EXPECT_EQ(4, queue.chainLength());
}

TEST(RedisEncoder, Encode) {
  RedisEncoder encoder;
  folly::IOBufEqual equal;

  RedisValue integer(123);
  EXPECT_TRUE(equal(folly::IOBuf::copyBuffer(":123\r\n"), encoder.encode(integer)));

  RedisValue error(RedisValue::Type::kError, "error");
  EXPECT_TRUE(equal(folly::IOBuf::copyBuffer("-error\r\n"), encoder.encode(error)));

  RedisValue simpleString(RedisValue::Type::kSimpleString, "string");
  EXPECT_TRUE(equal(folly::IOBuf::copyBuffer("+string\r\n"), encoder.encode(simpleString)));

  RedisValue bulkString(RedisValue::Type::kBulkString, "bulk\r\nstring");
  EXPECT_TRUE(equal(folly::IOBuf::copyBuffer("$12\r\nbulk\r\nstring\r\n"), encoder.encode(bulkString)));

  RedisValue bulkStringArray(std::vector<std::string>{"a", "b"});
  EXPECT_TRUE(equal(folly::IOBuf::copyBuffer("*2\r\n$1\r\na\r\n$1\r\nb\r\n"), encoder.encode(bulkStringArray)));
}

}  // namespace codec
