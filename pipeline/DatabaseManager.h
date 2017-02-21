#ifndef PIPELINE_DATABASEMANAGER_H_
#define PIPELINE_DATABASEMANAGER_H_

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "folly/Conv.h"
#include "glog/logging.h"
#include "murmurhash3/MurmurHash3.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

namespace pipeline {

class DatabaseManager {
 public:
  using ColumnFamilyMap = std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*>;
  using ColumnFamilyGroupMap = std::unordered_map<std::string, std::vector<rocksdb::ColumnFamilyHandle*>>;

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

  // TODO(yunjing): this is copied from pipeline::RedisHandler. Find a way to avoid duplication.
  static bool parseInt(const std::string& value, int64_t* intValue) {
    try {
      *intValue = folly::to<int64_t>(value);
      return true;
    } catch (folly::ConversionError&) {
      return false;
    }
  }

  // Shard a string using murmurhash32
  static int getShardNum(const std::string& shardKey, int shardCount, uint32_t seed = 0) {
    uint32_t hash = 0;
    MurmurHash3_x86_32(shardKey.data(), shardKey.size(), seed, &hash);
    return hash % shardCount;
  }

  // Escape non-printable characters, %, and ~ using percent-encoding for strings to be used as database keys
  static void escapeKeyStr(const std::string& str, std::string* out);

  DatabaseManager(const ColumnFamilyMap& columnFamilyMap, bool masterReplica, rocksdb::DB* db)
      : columnFamilyMap_(columnFamilyMap),
        columnFamilyGroupMap_(kEmptyColumnFamilyGroupMap),
        masterReplica_(masterReplica),
        db_(db),
        metadataColumnFamily_(CHECK_NOTNULL(getColumnFamily(metadataColumnFamilyName()))) {}

  DatabaseManager(const ColumnFamilyMap& columnFamilyMap, const ColumnFamilyGroupMap& columnFamilyGroupMap,
                  bool masterReplica, rocksdb::DB* db)
      : columnFamilyMap_(columnFamilyMap),
        columnFamilyGroupMap_(columnFamilyGroupMap),
        masterReplica_(masterReplica),
        db_(db),
        metadataColumnFamily_(CHECK_NOTNULL(getColumnFamily(metadataColumnFamilyName()))) {}

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

  const ColumnFamilyGroupMap& columnFamilyGroupMap() const { return columnFamilyGroupMap_; }

  const std::vector<rocksdb::ColumnFamilyHandle*>& getColumnFamilyGroup(const std::string& name) {
    auto it = columnFamilyGroupMap().find(name);
    CHECK(it != columnFamilyGroupMap().end());
    return it->second;
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
  static const ColumnFamilyGroupMap kEmptyColumnFamilyGroupMap;

  const ColumnFamilyMap& columnFamilyMap_;
  const ColumnFamilyGroupMap& columnFamilyGroupMap_;
  const bool masterReplica_;
  rocksdb::DB* db_;
  rocksdb::ColumnFamilyHandle* metadataColumnFamily_;
};

}  // namespace pipeline

#endif  // PIPELINE_DATABASEMANAGER_H_
