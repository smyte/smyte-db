#ifndef INFRA_SCHEDULEDTASKQUEUE_H_
#define INFRA_SCHEDULEDTASKQUEUE_H_

#include <atomic>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "glog/logging.h"
#include "infra/ScheduledTask.h"
#include "infra/ScheduledTaskProcessor.h"
#include "pipeline/DatabaseManager.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_batch_base.h"

namespace infra {

// A RocksDB-backed queue for scheduled tasks
class ScheduledTaskQueue {
 public:
  // Name for the column family storing scheduled tasks. Use static method instead of a variable to ensure
  // initialization ordering when referenced in a global variable context.
  static const std::string& columnFamilyName() {
    static std::string name = "scheduled-tasks";
    return name;
  }

  // Optimize the RocksDb column family used for scheduled tasks persistence.
  // The goal is to support total (ascending) order seek of timestamps represented by int64_t.
  static void optimizeColumnFamily(int _, rocksdb::ColumnFamilyOptions* options) {
    // timestamp is of fixed size, but don't use prefix_extractor here as we need total ordering, see
    // https://github.com/facebook/rocksdb/wiki/Prefix-Seek-API-Changes#prefix-seek-api
    // Therefore, we only need to setup block cache and bloom filter
    // NOTE: don't use point lookup optimization since it uses hash index
    rocksdb::BlockBasedTableOptions blockBasedOptions;
    // setup block cache
    blockBasedOptions.block_cache = rocksdb::NewLRUCache(static_cast<size_t>(32 * 1024 * 1024));
    // use bloom filter to reduce disk I/O
    blockBasedOptions.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
    // create block-based table
    options->table_factory.reset(rocksdb::NewBlockBasedTableFactory(blockBasedOptions));
  }

  // Persistence of scheduled tasks is maintained in the give column family of RocksDB.
  // Using a non-default column family allows to avoid key conflicts, but any column family, including the default
  // one, suffices here.
  ScheduledTaskQueue(std::shared_ptr<ScheduledTaskProcessor> processor,
                     std::shared_ptr<pipeline::DatabaseManager> databaseManager,
                     rocksdb::ColumnFamilyHandle* columnFamily)
      : processor_(processor),
        databaseManager_(databaseManager),
        columnFamily_(columnFamily),
        run_(true),
        outstandingTaskCount_(0) {}

  // Start the background thread for processing scheduled tasks
  void start();

  // Stop the run loop for checking pending tasks
  void stop() {
    run_ = false;
  }

  void destroy() {
    CHECK(executionThread_ != nullptr) << "Execution thread has not been created";

    stop();

    if (executionThread_ && executionThread_->joinable()) {
      executionThread_->joinable();
    }

    LOG(INFO) << "Scheduled tasks execution thread destroyed";
  }

  // Process one batch of pending tasks with a scheduled time up to the given maxTimestampMs.
  size_t batchProcessing(int64_t maxTimestampMs);

  // Scan pending tasks with a scheduled time up to the given maxTimestampMs and return how many are pending.
  // Optionally, pending tasks are copied into the given task vector, though tasks are NOT removed from the database.
  // Because task data may be copied, a limit parameter is provided to cap memory usage. 0 means unlimited.
  size_t scanPendingTasks(int64_t maxTimestampMs, size_t limit = 0, std::vector<ScheduledTask>* tasks = nullptr);

  // Schedule a task using the given write batch.
  // It is safe and cheap to call it many times for different tasks with the same WriteBatch.
  // NOTE: The owner of the write batch is responsible for committing the changes.
  void scheduleWithWriteBatch(const ScheduledTask& task, rocksdb::WriteBatchBase* writeBatch) {
    writeBatch->Put(columnFamily_, task.key(), task.value());
    // We may be over counting here because until the caller commits the write batch, the tasks are not persistent
    // in the database. In the case of error, e.g., the caller failed to commit the write batch, the expected behavior
    // is to terminate the program.
    outstandingTaskCount_++;
  }

  // Schedule a list of tasks using the given write batch.
  void scheduleWithWriteBatch(const std::vector<ScheduledTask>& tasks, rocksdb::WriteBatchBase* writeBatch) {
    for (const auto& task : tasks) {
      writeBatch->Put(columnFamily_, task.key(), task.value());
      outstandingTaskCount_++;
    }
  }

  // Same as scheduleWithWriteBatch but passing the opaque key/value to a custom function to generate task objects.
  // In addition, clients may also specify an optional kafka offset to indicate the version of the key/value pair.
  // Use -1 to represent null value for kafka offset.
  // Return the number tasks scheduled or -1 to indicate an error.
  int scheduleOpaqueWithWriteBatch(const std::string& opaqueKey, const std::string& opaqueValue, int64_t kafkaOffset,
                                   rocksdb::WriteBatchBase* writeBatch) {
    std::vector<ScheduledTask> tasks;
    int ret = processor_->generateTasks(opaqueKey, opaqueValue, kafkaOffset, &tasks);
    if (ret > 0) {
      LOG(INFO) << ret << " tasks generated";
      for (const auto& task : tasks) {
        scheduleWithWriteBatch(task, writeBatch);
      }
    }
    // NOTE: the actual number of tasks may be smaller than reported because user-defined generateTasks may generate
    // multiple tasks with conflicting keys. It's client's responsibility to deal with such conflict.
    return ret;
  }

  // Schedule a single task without batching. The task is committed to database if this function succeeds.
  // Return true when the task is scheduled successfully.
  bool schedule(const ScheduledTask& task) {
    rocksdb::WriteBatch writeBatch;
    scheduleWithWriteBatch(task, &writeBatch);
    rocksdb::Status status = databaseManager_->db()->Write(rocksdb::WriteOptions(), &writeBatch);
    if (status.ok()) {
      return true;
    } else {
      LOG(ERROR) << "Failed to scheduled a single Task: " << status.ToString();
      return false;
    }
  }

  // Same as schedule but generate and schedule one task as a single batch.
  // Return the number tasks scheduled or negative to indicate an error.
  int scheduleOpaque(const std::string& opaqueKey, const std::string& opaqueValue, int64_t kafkaOffset) {
    rocksdb::WriteBatch writeBatch;
    int ret = scheduleOpaqueWithWriteBatch(opaqueKey, opaqueValue, kafkaOffset, &writeBatch);
    if (ret <= 0) {
      return ret;
    }

    rocksdb::Status status = databaseManager_->db()->Write(rocksdb::WriteOptions(), &writeBatch);
    if (status.ok()) {
      return ret;
    } else {
      LOG(ERROR) << "Failed to schedule tasks with opaque key/value: " << status.ToString();
      return -1;
    }
  }

  // Return the outstanding tasks in the database, which may be larger than the actual value.
  // Use accurateOutstandingTaskCountSlow when more accurate counting is needed.
  size_t outstandingTaskCount() const {
    return outstandingTaskCount_;
  }

  // Accurate count of outstanding tasks in the database. It can be slow when there are many tasks pending.
  size_t accurateOutstandingTaskCountSlow() {
    return scanPendingTasks(std::numeric_limits<int64_t>::max());
  }

 private:
  // Check pending tasks every 1 second
  static constexpr int64_t kCheckIntervalMs = 1000;
  // Batch size limit for each scan
  static constexpr size_t kScanBatchSize = 10000;

  std::shared_ptr<ScheduledTaskProcessor> processor_;
  std::shared_ptr<pipeline::DatabaseManager> databaseManager_;
  rocksdb::ColumnFamilyHandle* columnFamily_;
  bool run_;
  std::atomic_size_t outstandingTaskCount_;
  // Background thread that executes tasks at schedule time.
  // TODO(yunjing): consider a multi-threaded design for higher throughput. The idea is to use one thread to pick up
  // pending tasks and then distribute the tasks to a pool of worker threads
  std::unique_ptr<std::thread> executionThread_;
};

}  // namespace infra

#endif  // INFRA_SCHEDULEDTASKQUEUE_H_
