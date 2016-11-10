#include <memory>
#include <string>
#include <vector>

#include "folly/Conv.h"
#include "gtest/gtest.h"
#include "infra/ScheduledTaskProcessor.h"
#include "infra/ScheduledTaskQueue.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"
#include "stesting/TestWithRocksDb.h"

namespace infra {

class ScheduledTaskQueueTest : public stesting::TestWithRocksDb {
 protected:
  ScheduledTaskQueueTest() : stesting::TestWithRocksDb({ "scheduled-tasks" }) {}
};

class TestScheduledTaskProcessor : public ScheduledTaskProcessor {
 public:
  void processPendingTasks(std::vector<ScheduledTask>* tasks, rocksdb::WriteBatch* writeBatch) override {
    for (auto& task : (*tasks)) {
      task.markCompleted();
    }
  }

  int generateTasks(const std::string& opaqueKey, const std::string& opaqueValue, int64_t kafkaOffset,
                    std::vector<ScheduledTask>* tasks) override {
    if (opaqueValue.empty()) {
      return 0;
    } else if (opaqueValue == "error") {
      return -1;
    } else {
      tasks->emplace_back(folly::to<int64_t>(opaqueValue), opaqueKey, "");
      return 1;
    }
  }
};

TEST_F(ScheduledTaskQueueTest, ScheduleWithWriteBatch) {
  ScheduledTask task1{ 1472295107012L, "key1", "value1" };
  ScheduledTask task2{ 1462295107012L, "key2", "value2" };
  ScheduledTask task3{ 1562295107512L, "key3", "value3" };

  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));
  rocksdb::WriteBatch writeBatch;
  queue.scheduleWithWriteBatch(task1, &writeBatch);
  queue.scheduleWithWriteBatch(task2, &writeBatch);
  queue.scheduleWithWriteBatch(task3, &writeBatch);
  commitWriteBatch(&writeBatch);
  EXPECT_EQ(3, queue.outstandingTaskCount());

  // small timestamp, no pending tasks found
  std::vector<ScheduledTask> pendingTasks;
  queue.scanPendingTasks(1402295107012L, 0, &pendingTasks);
  EXPECT_TRUE(pendingTasks.empty());

  // timestamp later than scheduled time for task1 and task2
  pendingTasks.clear();
  queue.scanPendingTasks(1482295107012L, 0, &pendingTasks);
  EXPECT_EQ(2, pendingTasks.size());
  EXPECT_EQ(task2, pendingTasks[0]);
  EXPECT_EQ(task1, pendingTasks[1]);

  // timestamp later than scheduled time for all tasks
  pendingTasks.clear();
  queue.scanPendingTasks(1682295107012L, 0, &pendingTasks);
  EXPECT_EQ(3, pendingTasks.size());
  EXPECT_EQ(task2, pendingTasks[0]);
  EXPECT_EQ(task1, pendingTasks[1]);
  EXPECT_EQ(task3, pendingTasks[2]);
}

TEST_F(ScheduledTaskQueueTest, ScheduleOpaqueWithWriteBatch) {
  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));

  rocksdb::WriteBatch writeBatch1;
  EXPECT_EQ(1, queue.scheduleOpaqueWithWriteBatch("key1", "1472295107012", -1, &writeBatch1));
  EXPECT_EQ(1, queue.scheduleOpaqueWithWriteBatch("key2", "1462295107012", -1, &writeBatch1));
  EXPECT_EQ(1, queue.scheduleOpaqueWithWriteBatch("key3", "1562295107512", -1, &writeBatch1));
  commitWriteBatch(&writeBatch1);
  EXPECT_EQ(3, queue.outstandingTaskCount());

  // with empty or error values
  rocksdb::WriteBatch writeBatch2;
  EXPECT_EQ(0, queue.scheduleOpaqueWithWriteBatch("key1", "", -1, &writeBatch2));
  EXPECT_EQ(-1, queue.scheduleOpaqueWithWriteBatch("key1", "error", -1, &writeBatch2));
  commitWriteBatch(&writeBatch2);
  // still have just 3 outstanding tasks
  EXPECT_EQ(3, queue.outstandingTaskCount());
}

TEST_F(ScheduledTaskQueueTest, Schedule) {
  ScheduledTask task1{ 1472295107012L, "key1", "value1" };
  ScheduledTask task2{ 1462295107012L, "key2", "value2" };
  ScheduledTask task3{ 1562295107512L, "key3", "value3" };

  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));
  queue.schedule(task1);
  queue.schedule(task2);
  queue.schedule(task3);
  EXPECT_EQ(3, queue.outstandingTaskCount());

  // small timestamp, no pending tasks found
  std::vector<ScheduledTask> pendingTasks;
  queue.scanPendingTasks(1402295107012L, 0, &pendingTasks);
  EXPECT_TRUE(pendingTasks.empty());

  // timestamp later than scheduled time for task1 and task2
  pendingTasks.clear();
  queue.scanPendingTasks(1482295107012L, 0, &pendingTasks);
  EXPECT_EQ(2, pendingTasks.size());
  EXPECT_EQ(task2, pendingTasks[0]);
  EXPECT_EQ(task1, pendingTasks[1]);

  // timestamp later than scheduled time for all tasks
  pendingTasks.clear();
  queue.scanPendingTasks(1682295107012L, 0, &pendingTasks);
  EXPECT_EQ(3, pendingTasks.size());
  EXPECT_EQ(task2, pendingTasks[0]);
  EXPECT_EQ(task1, pendingTasks[1]);
  EXPECT_EQ(task3, pendingTasks[2]);
}

TEST_F(ScheduledTaskQueueTest, ScheduleOpaque) {
  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));

  EXPECT_EQ(1, queue.scheduleOpaque("key1", "1472295107012", -1));
  EXPECT_EQ(1, queue.scheduleOpaque("key2", "1462295107012", -1));
  EXPECT_EQ(1, queue.scheduleOpaque("key3", "1562295107512", -1));
  EXPECT_EQ(3, queue.outstandingTaskCount());

  // with empty or error values
  EXPECT_EQ(0, queue.scheduleOpaque("key1", "", -1));
  EXPECT_EQ(-1, queue.scheduleOpaque("key1", "error", -1));
  // still have just 3 outstanding tasks
  EXPECT_EQ(3, queue.outstandingTaskCount());
}

TEST_F(ScheduledTaskQueueTest, ScanPendingTasksWithLimit) {
  ScheduledTask task1{ 1472295107012L, "key1", "value1" };
  ScheduledTask task2{ 1462295107012L, "key2", "value2" };
  ScheduledTask task3{ 1562295107512L, "key3", "value3" };

  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));
  rocksdb::WriteBatch writeBatch;
  queue.scheduleWithWriteBatch(task1, &writeBatch);
  queue.scheduleWithWriteBatch(task2, &writeBatch);
  queue.scheduleWithWriteBatch(task3, &writeBatch);
  commitWriteBatch(&writeBatch);
  EXPECT_EQ(3, queue.outstandingTaskCount());

  EXPECT_EQ(3, queue.scanPendingTasks(1682295107012L, 0));
  EXPECT_EQ(1, queue.scanPendingTasks(1682295107012L, 1));
  EXPECT_EQ(2, queue.scanPendingTasks(1682295107012L, 2));
  EXPECT_EQ(3, queue.scanPendingTasks(1682295107012L, 3));
}

TEST_F(ScheduledTaskQueueTest, BatchProcessing) {
  ScheduledTask task1{ 1472295107012L, "key1", "value1" };
  ScheduledTask task2{ 1462295107012L, "key2", "value2" };
  ScheduledTask task3{ 1562295107512L, "key3", "value3" };

  ScheduledTaskQueue queue(std::make_unique<TestScheduledTaskProcessor>(), databaseManager(),
                           columnFamily("scheduled-tasks"));
  queue.schedule(task1);
  queue.schedule(task2);
  queue.schedule(task3);
  EXPECT_EQ(3, queue.outstandingTaskCount());

  // all tasks become pending
  queue.batchProcessing(1682295107012L);
  EXPECT_EQ(0, queue.outstandingTaskCount());
}

}  // namespace infra
