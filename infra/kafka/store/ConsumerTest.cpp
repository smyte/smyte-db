#include "gtest/gtest.h"
#include "infra/kafka/store/Consumer.h"

namespace infra {
namespace kafka {
namespace store {

TEST(Consumer, GetObjectName) {
  EXPECT_EQ("abc/counters/000003/00000000000000012345", Consumer::getObjectName("abc/", "counters", 3, 12345));
}

}  // namespace store
}  // namespace kafka
}  // namespace infra
