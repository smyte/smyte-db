#include <chrono>
#include <limits>

#include "gtest/gtest.h"
#include "infra/SmyteId.h"

namespace infra {

TEST(SmyteIdTest, GetShardIndex) {
  EXPECT_EQ(4, SmyteId(12345).getShardIndex(20));
  EXPECT_EQ(14, SmyteId(12345).getShardIndex(15));

  EXPECT_EQ(3, SmyteId(123456789).getShardIndex(20));
  EXPECT_EQ(8, SmyteId(123456789).getShardIndex(9));

  EXPECT_EQ(4, SmyteId(std::numeric_limits<int64_t>::max()).getShardIndex(20));
  EXPECT_EQ(4, SmyteId(std::numeric_limits<int64_t>::max()).getShardIndex(10));
}

TEST(SmyteIdTest, AsBinary) {
  std::string smyteIdBinary = SmyteId(12345).asBinary();
  EXPECT_EQ(SmyteId(12345), SmyteId(smyteIdBinary));
}

TEST(SmyteIdTest, AppendAsBinary) {
  std::string smyteId;
  SmyteId(12345).appendAsBinary(&smyteId);
  EXPECT_EQ(SmyteId(12345), SmyteId(smyteId));
}

TEST(SmyteIdTest, GenerateFromKafkaBadInput) {
  EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampEpoch - 10, 10),
               "Check failed.*timestamp 1262303999990 for kafka offset 123 is out of range");
  EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampSize, 10),
               "Check failed.*timestamp 1099511627776 for kafka offset 123 is out of range");
  EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampSize + 10, 10),
               "Check failed.*timestamp 1099511627786 for kafka offset 123 is out of range");

  EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampEpoch + 100, -1),
               "Check failed.*virtual shard -1 for kafka offset 123 is out of range");
  EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampEpoch + 100, SmyteId::kVirtualShardCount),
               "Check failed.*virtual shard 1024 for kafka offset 123 is out of range");
    EXPECT_DEATH(SmyteId::generateFromKafka(123, SmyteId::kTimestampEpoch + 100, SmyteId::kVirtualShardCount + 10),
               "Check failed.*virtual shard 1034 for kafka offset 123 is out of range");
}

TEST(SmyteId, GenerateFromKafkaGoodInput) {
  EXPECT_EQ(SmyteId::generateFromKafka(0, SmyteId::kTimestampEpoch, 0), SmyteId(7168));
  EXPECT_EQ(SmyteId::generateFromKafka(123, SmyteId::kTimestampEpoch, 10), SmyteId(1014794));
  EXPECT_EQ(SmyteId::generateFromKafka(12345, SmyteId::kTimestampEpoch + 10, 1023), SmyteId(84361215));
  int64_t nowMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  EXPECT_EQ(SmyteId::generateFromKafka(123456, nowMs, 1000),
            SmyteId((nowMs - SmyteId::kTimestampEpoch) << (SmyteId::kUniqueBits + SmyteId::kMachineBits) |
                    (123456 % SmyteId::kUniqueSize) << SmyteId::kMachineBits |
                    (SmyteId::kMachineSize - SmyteId::kVirtualShardCount + 1000)));
}

}  // namespace infra
