#ifndef INFRA_KAFKA_STORE_CONSUMER_H_
#define INFRA_KAFKA_STORE_CONSUMER_H_

#include <memory>
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "avro/DataFile.hh"
#pragma GCC diagnostic pop
#include "boost/filesystem.hpp"
#include "folly/Format.h"
#include "folly/String.h"
#include "glog/logging.h"
#include "infra/kafka/AbstractConsumer.h"
#include "infra/kafka/OffsetManager.h"
#include "infra/kafka/store/KafkaStoreMessageRecord.hh"
#include "librdkafka/rdkafkacpp.h"
#include "platform/gcloud/GoogleCloudStorage.h"

namespace infra {
namespace kafka {
namespace store {

// Consumer for kafka-store
class Consumer : public infra::kafka::AbstractConsumer {
 public:
  static std::string getObjectName(const std::string& objectNamePrefix, const std::string& topic, int partition,
                                   int64_t fileOffset) {
    // partition has up to 6 digits and offset has up to 20
    return folly::sformat("{}{}/{:06d}/{:020d}", objectNamePrefix, topic, partition, fileOffset);
  }

  // Technically, a kafka-store consumer does not depend on kafka cluster since it reads kafka messages directly from
  // cold storage. However, we still use kafka cluster to store current offset to ease metrics reporting
  // TODO(yunjing): define an interface for cold storage and support other solutions such as S3
  Consumer(const std::string& brokerList, const std::string& bucketName, const std::string& objectNamePrefix,
           const std::string& topic, int partition, const std::string& groupId, const std::string& offsetKey,
           std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper,
           std::shared_ptr<platform::gcloud::GoogleCloudStorage> gcs)
      : infra::kafka::AbstractConsumer(offsetKey, consumerHelper),
        brokerList_(brokerList),
        bucketName_(bucketName),
        objectNamePrefix_(objectNamePrefix),
        topic_(topic),
        partition_(partition),
        groupId_(groupId),
        gcs_(gcs),
        currentFilePath_(""),
        currentDataReader_(nullptr),
        currentFileOffset_(0),
        nextFileOffset_(0),
        nextKafkaOffset_(0),
        offsetManager_(brokerList, topic, partition, groupId) {}

  virtual ~Consumer() {}

  void init(int64_t initialOffset) override;

  void destroy(void) override {
    LOG(INFO) << "Stopping kafka store consumer";
    // stop the infinite loop in the consumer thread
    stop();

    waitForStop();

    boost::filesystem::remove(currentFilePath_);
    currentFilePath_ = "";
    if (currentDataReader_) {
      currentDataReader_->close();
      currentDataReader_.reset(nullptr);
    }

    LOG(INFO) << "Kafka store consumer destroyed";
  }

  // Load the kafka and file offsets that have been previously committed
  // Return value indicates if the operation succeeded
  virtual bool loadCommittedKafkaAndFileOffsets(int64_t* kafkaOffset, int64_t* fileOffset) {
    return consumerHelper()->loadCommittedKafkaAndFileOffsetsFromDb(offsetKey(), kafkaOffset, fileOffset);
  }

  int64_t loadCommittedKafkaOffset(void) override {
    int64_t kafkaOffset = RdKafka::Topic::OFFSET_INVALID;
    if (loadCommittedKafkaAndFileOffsets(&kafkaOffset, nullptr)) {
      return kafkaOffset;
    } else {
      return RdKafka::Topic::OFFSET_INVALID;
    }
  }

  // Process a batch of messages.
  //
  // Subclasses may override it to pass in an opaque object for processing each batch of messages
  // one useful opaque object would be rocksdb::WriteBatch, which allows individual messages to add
  // key/value pairs to the batch but only write to database at the end of batch processing.
  //
  // Overriding method must call consumeBatch.
  void processBatch(int timeoutMs) override {
    consumeBatch(timeoutMs, nullptr);
  }

  // Process one message.
  virtual void processOne(int64_t offset, const KafkaStoreMessage& msg, void* opaque) = 0;

 protected:
  size_t consumeBatch(int timeoutMs, void* opaque);

  // Commit the next offset to kafka cluster for metrics and reporting
  bool commitSync(void) {
    return offsetManager_.commitOffset(nextKafkaOffset_, false);
  }
  bool commitAsync(void) {
    return offsetManager_.commitOffset(nextKafkaOffset_, true);
  }

  int64_t currentFileOffset(void) const {
    return currentFileOffset_;
  }

  int64_t nextFileOffset(void) const {
    return nextFileOffset_;
  }

  // WARN: return value is undefined when called from within `processOne`
  int64_t nextKafkaOffset(void) const {
    return nextKafkaOffset_;
  }

 private:
  static constexpr size_t kMaxBatchSize = 10000;
  static constexpr char kDownloadPathTemplate[] = "/tmp/kafka-store.%%%%%%%%";

  // Download the target object specified by the file offset and fill the metadata object.
  // Return the number of records in the object or -1 on failure
  int64_t downloadObjectAndValidate(int64_t fileOffset, const std::string& downloadPath,
                                    google_storage_api::Object* object);

  // Download the file with the given offset update and update the output path with download destination.
  // Return record count of the file. -1 indicates error.
  int64_t downloadFile(int64_t fileOffset, bool retry, std::string* path);

  const std::string brokerList_;
  const std::string bucketName_;
  const std::string objectNamePrefix_;
  const std::string topic_;
  const int partition_;
  const std::string groupId_;
  std::shared_ptr<platform::gcloud::GoogleCloudStorage> gcs_;
  std::string currentFilePath_;
  std::unique_ptr<avro::DataFileReader<KafkaStoreMessage>> currentDataReader_;
  int64_t currentFileOffset_;
  int64_t nextFileOffset_;
  int64_t nextKafkaOffset_;
  infra::kafka::OffsetManager offsetManager_;
};

}  // namespace store
}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_STORE_CONSUMER_H_
