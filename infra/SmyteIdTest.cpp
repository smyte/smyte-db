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

}  // namespace infra
