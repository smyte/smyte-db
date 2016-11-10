#include <string>

#include "gtest/gtest.h"
#include "infra/ScheduledTask.h"
#include "rocksdb/slice.h"

namespace infra {

TEST(ScheduledTaskTest, EncodeTimestamp) {
  std::string buf1;
  ScheduledTask::encodeTimestamp(0x1000001234567890L, &buf1);
  EXPECT_EQ(sizeof(int64_t), buf1.size());
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[0]), 0x10);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[1]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[2]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[3]), 0x12);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[4]), 0x34);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[5]), 0x56);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[6]), 0x78);
  EXPECT_EQ(static_cast<unsigned char>(buf1.data()[7]), 0x90);

  std::string buf2;
  ScheduledTask::encodeTimestamp(0x7fffffffffffffffL, &buf2);
  EXPECT_EQ(sizeof(int64_t), buf2.size());
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[0]), 0x7f);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[1]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[2]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[3]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[4]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[5]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[6]), 0xff);
  EXPECT_EQ(static_cast<unsigned char>(buf2.data()[7]), 0xff);
}

TEST(ScheduledTaskTest, DecodeTimestamp) {
  unsigned char buf1[] = { 0x10, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90 };
  EXPECT_EQ(0x1000001234567890L, ScheduledTask::decodeTimestamp(reinterpret_cast<char*>(buf1)));

  unsigned char buf2[] = { 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  EXPECT_EQ(0x7fffffffffffffffL, ScheduledTask::decodeTimestamp(reinterpret_cast<char*>(buf2)));
}

TEST(ScheduledTaskTest, EncodedTimestampComparison) {
  std::string buf1;
  rocksdb::Slice slice1 = ScheduledTask::encodeTimestamp(1462295107012L, &buf1);
  std::string buf2;
  rocksdb::Slice slice2 = ScheduledTask::encodeTimestamp(1562295107512L, &buf2);
  std::string buf3;
  rocksdb::Slice slice3 = ScheduledTask::encodeTimestamp(2462295107102L, &buf3);

  EXPECT_GT(0, slice1.compare(slice2));
  EXPECT_LT(0, slice3.compare(slice2));
  EXPECT_EQ(0, slice2.compare(slice2));
}

TEST(ScheduledTaskTest, TaskKey) {
  ScheduledTask task1{ 0x1234L, "key1", "value1" };
  ScheduledTask task2{ 0x56780000L, "key2", "value2" };

  EXPECT_EQ(std::string({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 'k', 'e', 'y', '1'}), task1.key());
  EXPECT_EQ(std::string({0x00, 0x00, 0x00, 0x00, 0x56, 0x78, 0x00, 0x00, 'k', 'e', 'y', '2'}), task2.key());
}

TEST(ScheduledTaskTest, TaskComparison) {
  ScheduledTask task1{ 1472295107012L, "key1", "value1" };
  ScheduledTask task2{ 1462295107012L, "key2", "value2" };
  ScheduledTask task3{ 1472295107012L, "key1", "value1" };

  // different tasks
  EXPECT_NE(task1, task2);
  EXPECT_NE(task2, task3);
  // self-comparison
  EXPECT_EQ(task1, task1);
  EXPECT_EQ(task2, task2);
  EXPECT_EQ(task3, task3);
  // different instances of same value
  EXPECT_EQ(task1, task3);
}

TEST(ScheduledTaskTest, MarkCompleted) {
  ScheduledTask task{ 1472295107012L, "key", "value" };

  EXPECT_FALSE(task.completed());
  task.markCompleted();
  EXPECT_TRUE(task.completed());
}

}  // namespace infra
