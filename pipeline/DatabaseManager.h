#ifndef PIPELINE_DATABASEMANAGER_H_
#define PIPELINE_DATABASEMANAGER_H_

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "glog/logging.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

namespace pipeline {

class DatabaseManager {
 public:
  using ColumnFamilyMap = std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*>;

  static const char* defaultColumnFamilyName() {
    return "default";
  }

  static const char* metadataColumnFamilyName() {
    return "smyte-metadata";
  }

  static rocksdb::Slice encodeInt64(int64_t value, std::string* buf) {
    buf->append(reinterpret_cast<const char *>(&value), sizeof(value));
    return rocksdb::Slice(*buf);
  }

  static bool decodeInt64(const rocksdb::Slice& value, int64_t* intValue) {
    if (value.size() != sizeof(*intValue)) return false;

    std::memcpy(intValue, value.data(), sizeof(*intValue));
    return true;
  }

  DatabaseManager(const ColumnFamilyMap& columnFamilyMap, bool masterReplica, rocksdb::DB* db)
      : columnFamilyMap_(columnFamilyMap), masterReplica_(masterReplica), db_(db), metadataColumnFamily_(nullptr) {
    metadataColumnFamily_ = CHECK_NOTNULL(getColumnFamily(metadataColumnFamilyName()));
  }

  virtual ~DatabaseManager() {}

  virtual void start() {}
  virtual void destroy() {}

  rocksdb::DB* db() const { return db_; }

  const ColumnFamilyMap& columnFamilyMap() const { return columnFamilyMap_; }

  rocksdb::ColumnFamilyHandle* getMetadataColumnFamily() const { return metadataColumnFamily_; }

  rocksdb::ColumnFamilyHandle* getColumnFamily(const std::string& columnFamilyName) {
    auto entry = columnFamilyMap().find(columnFamilyName);
    return entry != columnFamilyMap().end() ? entry->second : nullptr;
  }

  bool freeze(std::vector<std::string>* fileList);

  bool thaw() {
    rocksdb::Status status = db_->EnableFileDeletions();
    if (!status.ok()) {
      LOG(ERROR) << "RocksDB EnableFileDeletions Error: " << status.ToString();
      return false;
    }
    return true;
  }

  bool forceCompaction(rocksdb::ColumnFamilyHandle* columnFamily) {
    rocksdb::CompactRangeOptions options;
    // allow moving data files back to the minimum level capable of holding the data
    options.change_level = true;
    // make sure all levels are forced to compact
    options.bottommost_level_compaction = rocksdb::BottommostLevelCompaction::kForce;
    rocksdb::Status status = db_->CompactRange(options, columnFamily, nullptr, nullptr);
    if (!status.ok()) {
      LOG(ERROR) << "RocksDB CompactRange Error: " << status.ToString();
      return false;
    }

    return true;
  }

  bool isMasterReplica() const {
    return masterReplica_;
  }

 private:
  const ColumnFamilyMap& columnFamilyMap_;
  const bool masterReplica_;
  rocksdb::DB* db_;
  rocksdb::ColumnFamilyHandle* metadataColumnFamily_;
};

}  // namespace pipeline

#endif  // PIPELINE_DATABASEMANAGER_H_
