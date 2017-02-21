#include "infra/ScheduledTaskQueue.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "glog/logging.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"

namespace infra {

using std::chrono::milliseconds;

void ScheduledTaskQueue::start() {
  CHECK(executionThread_ == nullptr) << "Execution thread already started";

  CHECK_EQ(outstandingTaskCount_, 0);
  outstandingTaskCount_ = accurateOutstandingTaskCountSlow();

  executionThread_.reset(new std::thread([this]() {
    while (this->run_) {
      // scan up to the next millisecond
      int64_t maxTimestampMs =
        std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1;
      // loop until the queue is exhausted
      while (this->batchProcessing(maxTimestampMs) == kScanBatchSize) {}
      std::this_thread::sleep_for(milliseconds(kCheckIntervalMs));
    }
  }));

  LOG(INFO) << "ScheduledTaskQueue execution thread started";
}

size_t ScheduledTaskQueue::batchProcessing(int64_t maxTimestampMs) {
  std::vector<ScheduledTask> tasks;
  size_t count = scanPendingTasks(maxTimestampMs, kScanBatchSize, &tasks);
  if (count > 0) {
    DLOG(INFO) << "Found " << count << " pending tasks";
    rocksdb::WriteBatch writeBatch;
    processor_->processPendingTasks(&tasks, &writeBatch);

    size_t numCompleted = 0;
    for (const auto& task : tasks) {
      if (task.completed()) {
        numCompleted++;
        writeBatch.Delete(columnFamily_, task.key());
      }
    }
    rocksdb::Status status = databaseManager_->db()->Write(rocksdb::WriteOptions(), &writeBatch);
    CHECK(status.ok()) << "Fail to persist results of scheduled task processing: " << status.ToString();

    outstandingTaskCount_ -= numCompleted;
    if (numCompleted < tasks.size()) {
      // not all pending tasks completed, they will be retried in next batch.
      // TODO(yunjing): report the lag of processing pending tasks and repeatedly retried failed tasks
      LOG(WARNING) << tasks.size() - numCompleted << " out of " << tasks.size() << " pending tasks not completed";
    } else {
      DLOG(INFO) << "Completed " << numCompleted << " pending tasks";
    }
  }
  return count;
}

size_t ScheduledTaskQueue::scanPendingTasks(int64_t maxTimestampMs, size_t limit, std::vector<ScheduledTask>* tasks) {
  rocksdb::ReadOptions readOptions;
  readOptions.total_order_seek = true;  // unnecessary as long as not using hash index; keep it here for safety
  std::string buf;
  rocksdb::Slice maxTimestamp = ScheduledTask::encodeTimestamp(maxTimestampMs, &buf);
  readOptions.iterate_upper_bound = &maxTimestamp;
  auto iter = std::unique_ptr<rocksdb::Iterator>(databaseManager_->db()->NewIterator(readOptions, columnFamily_));
  size_t count = 0;
  // seek from beginning until reaching maxTimestampMs
  for (iter->SeekToFirst(); iter->Valid() && (limit == 0 || count < limit); iter->Next()) {
    // task key <=> (timestamp, data key)
    rocksdb::Slice taskKey = iter->key();
    rocksdb::Slice timestamp = taskKey;
    timestamp.remove_suffix(taskKey.size() - sizeof(maxTimestampMs));
    rocksdb::Slice dataKey = taskKey;
    dataKey.remove_prefix(sizeof(maxTimestampMs));

    count++;
    if (tasks) {
      tasks->emplace_back(ScheduledTask::decodeTimestamp(timestamp.data()), dataKey.ToString(),
                          iter->value().ToString());
    }
  }

  return count;
}

constexpr int64_t ScheduledTaskQueue::kCheckIntervalMs;
constexpr size_t ScheduledTaskQueue::kScanBatchSize;

}  // namespace infra
