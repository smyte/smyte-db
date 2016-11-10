#include "infra/kafka/store/Consumer.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <utility>

#include "boost/filesystem.hpp"
#include "folly/Format.h"
#include "glog/logging.h"
#include "infra/kafka/store/KafkaStoreMessageRecord.hh"
#include "librdkafka/rdkafkacpp.h"
#include "platform/gcloud/GoogleCloudStorage.h"

namespace infra {
namespace kafka {
namespace store {

using google_storage_api::Object;

int64_t Consumer::downloadObjectAndValidate(int64_t fileOffset, const std::string& downloadPath, Object* object) {
  std::string objectName = getObjectName(objectNamePrefix_, topic_, partition_, fileOffset);
  LOG(INFO) << "Downloading object " << objectName << " in " << bucketName_;
  googleapis::util::Status status = gcs_->downloadObject(bucketName_, objectName, downloadPath, object);
  if (!status.ok()) {
    LOG(ERROR) << status.ToString();
    return -1;
  }
  if (!object->has_metadata()) {
    LOG(ERROR) << "Object has no metadata";
    return -1;
  }
  std::string countValue;
  if (!object->get_metadata().get("count", &countValue)) {
    LOG(ERROR) << "Object has not `count` metadata";
    return -1;
  }
  int64_t count = -1;
  try {
    count = folly::to<int64_t>(countValue);
  } catch (folly::ConversionError&) {
    LOG(ERROR) << "Failed to convert count value: " << countValue;
    return -1;
  }
  if (count <= 0) {
    LOG(ERROR) << "Invalid count: " << count;
    return -1;
  }

  LOG(INFO) << "Downloaded object " << objectName << " in " << bucketName_ << " with " << count << " records";
  return count;
}

void Consumer::init(int64_t initialOffset) {
  offsetManager_.init();

  CHECK(initialOffset >= 0 || initialOffset == RdKafka::Topic::OFFSET_STORED)
      << "Either specify a valid initial kafka offset or use RdKafka::Topic::OFFSET_STORED";
  int64_t committedKafkaOffset = RdKafka::Topic::OFFSET_INVALID, initialFileOffset = RdKafka::Topic::OFFSET_INVALID;
  CHECK(loadCommittedKafkaAndFileOffsets(&committedKafkaOffset, &initialFileOffset));
  if (initialOffset == RdKafka::Topic::OFFSET_STORED) {
    initialOffset = committedKafkaOffset;
  }
  CHECK(initialFileOffset >= 0) << "Invalid initial file offset: " << initialFileOffset;
  CHECK(initialOffset >= initialFileOffset) << "Invalid combination of kafka offset " << initialOffset
                                            << " and file offset: " << initialFileOffset;

  // download the initial file without retry
  int64_t recordCount = downloadFile(initialFileOffset, false, &currentFilePath_);
  CHECK(recordCount > 0 && initialFileOffset + recordCount > initialOffset)
      << "Failed to download or validate object for initial offset " << initialOffset << " and initial file offset "
      << initialFileOffset;
  currentDataReader_.reset(new avro::DataFileReader<KafkaStoreMessage>(currentFilePath_.data()));
  currentFileOffset_ = initialFileOffset;
  nextFileOffset_ = currentFileOffset_ + recordCount;
  if (initialOffset != initialFileOffset) {
    // need to seek to initial offset
    int64_t currentOffset = initialFileOffset;
    KafkaStoreMessage msg;
    while (currentOffset < initialOffset) {
      CHECK(currentDataReader_->read(msg));
      currentOffset++;
    }
  }
  nextKafkaOffset_ = initialOffset;

  LOG(INFO) << "Start consuming partition " << partition_ << " of " << topic_ << " as " << groupId_ << " from offset "
            << nextKafkaOffset_;
  setInitialized();
}

size_t Consumer::consumeBatch(int timeoutMs, void* opaque) {
  if (!run()) return 0;

  if (!currentDataReader_) {
    CHECK_EQ(nextKafkaOffset_, nextFileOffset_)
        << "Kafka offset and file offset must match when starting a new file";
    int64_t recordCount = downloadFile(nextFileOffset_, true, &currentFilePath_);
    if (!run()) return 0;
    CHECK_GT(recordCount, 0);
    currentDataReader_.reset(new avro::DataFileReader<KafkaStoreMessage>(currentFilePath_.data()));
    currentFileOffset_ = nextFileOffset_;
    nextFileOffset_ += recordCount;
  }

  // start the timer after a file is downloaded, which may take more than timeoutMs
  size_t count = 0;
  int64_t start = nowMs();
  int remainingMs = timeoutMs;
  while (run() && count < kMaxBatchSize && remainingMs > 0) {
    KafkaStoreMessage msg;
    if (currentDataReader_->read(msg)) {
      processOne(nextKafkaOffset_, msg, opaque);
      count++;
      nextKafkaOffset_++;

      // need to update file offset at the same time
      if (nextKafkaOffset_ >= nextFileOffset_) {
        // current file exhausted, wrap up the batch
        CHECK_EQ(nextKafkaOffset_, nextFileOffset_)
            << "Kafka offset and file offset must match when starting a new file";
        currentDataReader_->close();
        currentDataReader_.reset(nullptr);
        boost::filesystem::remove(currentFilePath_);
        currentFilePath_ = "";
        break;
      }
    } else {
      LOG(FATAL) << "Record count in kafka store file " << currentFileOffset_ << " is inconsistent with its metadata";
    }
    remainingMs = timeoutMs - (nowMs() - start);
  }
  return count;
}

int64_t Consumer::downloadFile(int64_t fileOffset, bool retry, std::string* path) {
  while (run()) {
    std::string newPath = boost::filesystem::unique_path(kDownloadPathTemplate).native();
    auto object = std::unique_ptr<Object>(Object::New());
    int64_t newRecordCount = downloadObjectAndValidate(fileOffset, newPath, object.get());
    if (newRecordCount > 0) {
      // downloaded a valid file
      *path = std::move(newPath);
      return newRecordCount;
    }
    if (retry) {
      // After a successful initialization, a failed download usually indicates either transient system errors or
      // that the object has not been uploaded yet. So keep trying.
      LOG(WARNING) << "Kafka store file for " << fileOffset << " is not available. Retry in 60 seconds";
      // WARN: retry too frequently may incur an unexpected cost on cloud storage
      std::this_thread::sleep_for(std::chrono::seconds(60));
    } else {
      break;
    }
  }
  return -1;
}

constexpr char Consumer::kDownloadPathTemplate[];

}  // namespace store
}  // namespace kafka
}  // namespace infra
