#include "infra/kafka/ConsumerHelper.h"

#include <exception>
#include <string>

#include "folly/Conv.h"
#include "folly/dynamic.h"
#include "folly/json.h"
#include "librdkafka/rdkafkacpp.h"
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_batch_base.h"

namespace infra {
namespace kafka {

bool ConsumerHelper::commitRawOffsetValueWithWriteBatch(const std::string& offsetKey, const std::string& encodedOffset,
                                                        rocksdb::WriteBatchBase* writeBatch) {
  rocksdb::Status status;
  if (writeBatch) {
    writeBatch->Put(smyteMetadataCfHandle_, offsetKey, encodedOffset);
    status = db_->Write(rocksdb::WriteOptions(), writeBatch->GetWriteBatch());
  } else {
    status = db_->Put(rocksdb::WriteOptions(), smyteMetadataCfHandle_, offsetKey, encodedOffset);
  }

  if (!status.ok()) {
    LOG(ERROR) << "Persisting WriteBatch failed: " << status.ToString();
    return false;
  }

  return true;
}

int64_t ConsumerHelper::loadCommittedOffsetFromDb(const std::string& offsetKey) {
  std::string value;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), smyteMetadataCfHandle_, offsetKey, &value);

  if (status.ok()) {
    return setLastCommittedOffset(offsetKey, decodeOffset(value));
  } else {
    if (status.IsNotFound()) {
      LOG(WARNING) << "No committed offset found in DB";
    } else {
      LOG(ERROR) << "Reading offset from rocskdb failed: " << status.ToString();
    }
    return setLastCommittedOffset(offsetKey, RdKafka::Topic::OFFSET_INVALID);
  }
}

bool ConsumerHelper::loadCommittedKafkaAndFileOffsetsFromDb(const std::string& offsetKey, int64_t* kafkaOffset,
                                                            int64_t* fileOffset) {
  std::string value;
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), smyteMetadataCfHandle_, offsetKey, &value);
  if (status.ok()) {
    if (decodeKafkaAndFileOffsets(value, kafkaOffset, fileOffset)) {
      setLastCommittedOffset(offsetKey, *kafkaOffset);
      return true;
    }
  } else {
    if (status.IsNotFound()) {
      LOG(WARNING) << "No committed offsets found in DB";
    } else {
      LOG(ERROR) << "Reading offsets from rocskdb failed: " << status.ToString();
    }
  }
  setLastCommittedOffset(offsetKey, RdKafka::Topic::OFFSET_INVALID);
  return false;
}

int64_t ConsumerHelper::parseHighWatermarkOffset(const std::string& statsJson, const std::string& topic,
                                              int64_t partition) {
  try {
    folly::dynamic stats = folly::parseJson(statsJson);
    return stats["topics"][topic]["partitions"][folly::to<std::string>(partition)]["hi_offset"].getInt();
  } catch (const std::exception& e) {
    LOG(WARNING) << "Parsing kafka stats JSON failed: " << e.what();
    return RdKafka::Topic::OFFSET_INVALID;
  }
}

constexpr char ConsumerHelper::kKafkaAndFileOffsetsFormat[];

}  // namespace kafka
}  // namespace infra
