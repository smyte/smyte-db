#ifndef INFRA_SCHEDULEDTASKPROCESSOR_H_
#define INFRA_SCHEDULEDTASKPROCESSOR_H_

#include <string>
#include <vector>

#include "infra/ScheduledTask.h"
#include "rocksdb/write_batch.h"

namespace infra {

// Define key operations needed by ScheduledTasksQueues for processing batches of tasks
class ScheduledTaskProcessor {
 public:
  virtual ~ScheduledTaskProcessor() {}

  // Process a batch of pending tasks. Successfully processed tasks should be marked as completed.
  // The given write batch allows atomic operations on both task processing and the task completion in one transaction.
  virtual void processPendingTasks(std::vector<ScheduledTask>* tasks, rocksdb::WriteBatch* writeBatch) = 0;

  // Generate task objects from an opaque key/value fair.
  // Return number of tasks generated. Negative values indicate errors.
  // This is optional since clients may generate tasks and feed them to the scheduled task queue directly
  virtual int generateTasks(const std::string& opaqueKey, const std::string& opaqueValue, int64_t kafkaOffset,
                            std::vector<ScheduledTask>* tasks) {
    LOG(FATAL) << "Must override generateTasks to generate tasks from opaque key/value pair";
    return -1;
  }

  // Defines the maximum batch size the task processor is able to process
  virtual size_t getMaxBatchSize() const {
    return 10000;
  }
};

}  // namespace infra

#endif  // INFRA_SCHEDULEDTASKPROCESSOR_H_
