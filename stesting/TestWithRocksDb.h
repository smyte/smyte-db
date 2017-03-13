#ifndef STESTING_TESTWITHROCKSDB_H_
#define STESTING_TESTWITHROCKSDB_H_

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/filesystem.hpp"
#include "folly/Format.h"
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "pipeline/DatabaseManager.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"

namespace stesting {

class TestWithRocksDb : public ::testing::Test {
 protected:
  // Function to configure a column family in RocksDB, given a defaultBlockCacheSizeMb
  using RocksDbCfConfigurator = void (*)(int, rocksdb::ColumnFamilyOptions*);
  // Map column family names to RocksDbCfConfigurators
  using RocksDbCfConfiguratorMap = std::unordered_map<std::string, RocksDbCfConfigurator>;
  // Map column family group name to group count
  using RocksDbCfGroupConfigMap = std::unordered_map<std::string, size_t>;

  static void SetUpTestCase() {
    static bool initialized = false;
    if (!initialized) {
      FLAGS_logtostderr = true;
      google::InstallFailureSignalHandler();
      google::InitGoogleLogging("stesting::TestWithRocksDb");
      initialized = true;
    }
  }

  static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  // Provide optional column families and their configurators. `default` and `smyte-metadata` are always created
  // Column families named in optionalColumnFamilyNames will either be created with a provided configurator or use a
  // default configuration. Either, they are guaranteed to exist.
  explicit TestWithRocksDb(std::vector<std::string> optionalColumnFamilyNames = {},
                           RocksDbCfConfiguratorMap rocksDbCfConfiguratorMap = RocksDbCfConfiguratorMap(),
                           RocksDbCfGroupConfigMap rocksDbCfGroupConfigMap = RocksDbCfGroupConfigMap())
      : columnFamilyNames_(std::move(optionalColumnFamilyNames)),
        rocksDbCfConfiguratorMap_(std::move(rocksDbCfConfiguratorMap)),
        rocksDbCfGroupConfigMap_(std::move(rocksDbCfGroupConfigMap)) {
    // add the required column family names
    columnFamilyNames_.emplace_back("default");
    columnFamilyNames_.emplace_back("smyte-metadata");
  }

  void SetUp() override {
    dbPath_ = boost::filesystem::unique_path("rocksdb_test.%%%%%%%%");
    rocksdb::Options options;
    options.create_if_missing = true;

    // options for default column family
    std::vector<rocksdb::ColumnFamilyDescriptor> columnFamilyDescriptors;
    rocksdb::ColumnFamilyOptions defaultColumnFamilyOptions(options);
    if (rocksDbCfConfiguratorMap_.count("default") > 0) {
      // 1MB default cache, it makes no difference for testing really
      rocksDbCfConfiguratorMap_["default"](1, &defaultColumnFamilyOptions);
    }
    columnFamilyDescriptors.emplace_back("default", defaultColumnFamilyOptions);

    // create database with default column families
    std::vector<rocksdb::ColumnFamilyHandle*> columnFamilyHandles;
    rocksdb::Status status =
        rocksdb::DB::Open(options, dbPath_.native(), columnFamilyDescriptors, &columnFamilyHandles, &db_);
    CHECK(status.ok()) << "Fail to open rocksdb using temp directory: " << status.ToString();
    CHECK_EQ(columnFamilyHandles.size(), 1);
    columnFamilyMap_["default"] = columnFamilyHandles[0];

    // create the rest of the column families
    for (const auto& name : columnFamilyNames_) {
      if (name == "default") continue;

      rocksdb::ColumnFamilyOptions columnFamilyOptions(options);
      if (rocksDbCfConfiguratorMap_.count(name) > 0) {
        // 1MB default cache, it makes no difference for testing really
        rocksDbCfConfiguratorMap_[name](1, &columnFamilyOptions);
      }
      if (rocksDbCfGroupConfigMap_.count(name) > 0) {
        // create column family group
        std::vector<rocksdb::ColumnFamilyHandle*> group;
        for (size_t i = 0; i < rocksDbCfGroupConfigMap_[name]; i++) {
          auto cfName = folly::sformat("{}-{}", name, i);
          rocksdb::ColumnFamilyHandle* columnFamily;
          rocksdb::Status s = db_->CreateColumnFamily(columnFamilyOptions, cfName, &columnFamily);
          CHECK(s.ok()) << "Creating column family `" << cfName << "` failed: " << s.ToString();
          columnFamilyMap_[cfName] = columnFamily;
          group.push_back(columnFamily);
        }
        columnFamilyGroupMap_[name] = std::move(group);
      } else {
        // create a standalone column family
        rocksdb::ColumnFamilyHandle* columnFamily;
        rocksdb::Status s = db_->CreateColumnFamily(columnFamilyOptions, name, &columnFamily);
        CHECK(s.ok()) << "Creating column family `" << name << "` failed: " << s.ToString();
        columnFamilyMap_[name] = columnFamily;
      }
    }

    if (columnFamilyGroupMap_.empty()) {
      databaseManager_ = newDatabaseManager(columnFamilyMap_, db_);
    } else {
      databaseManager_ = newDatabaseManager(columnFamilyMap_, columnFamilyGroupMap_, db_);
    }
  }

  void TearDown() override {
    for (auto& entry : columnFamilyMap_) {
      db_->DestroyColumnFamilyHandle(entry.second);
    }
    columnFamilyMap_.clear();
    delete db_;
    db_ = nullptr;
    boost::filesystem::remove_all(dbPath_);
  }

  virtual std::shared_ptr<pipeline::DatabaseManager> newDatabaseManager(
      const pipeline::DatabaseManager::ColumnFamilyMap& columnFamilyMap, rocksdb::DB* db) {
    return std::make_shared<pipeline::DatabaseManager>(columnFamilyMap, true, db);
  }

  virtual std::shared_ptr<pipeline::DatabaseManager> newDatabaseManager(
      const pipeline::DatabaseManager::ColumnFamilyMap& columnFamilyMap,
      const pipeline::DatabaseManager::ColumnFamilyGroupMap& columnFamilyGroupMap, rocksdb::DB* db) {
    return std::make_shared<pipeline::DatabaseManager>(columnFamilyMap, columnFamilyGroupMap, true, db);
  }

  rocksdb::DB* db() const {
    return db_;
  }

  std::shared_ptr<pipeline::DatabaseManager> databaseManager() {
    return databaseManager_;
  }

  rocksdb::ColumnFamilyHandle* metadataColumnFamily() {
    return columnFamily("smyte-metadata");
  }

  rocksdb::ColumnFamilyHandle* columnFamily(const std::string& name) {
    CHECK_GT(columnFamilyMap_.count(name), 0);
    return columnFamilyMap_[name];
  }

  rocksdb::ColumnFamilyHandle* columnFamily(const std::string& groupName, int index) {
    CHECK_GT(columnFamilyGroupMap_.count(groupName), 0);
    CHECK_GT(columnFamilyGroupMap_[groupName].size(), index);
    return columnFamilyGroupMap_[groupName][index];
  }

  size_t totalKeyCount(rocksdb::ColumnFamilyHandle* columnFamilyHandle = nullptr) const {
    if (columnFamilyHandle == nullptr) columnFamilyHandle = db()->DefaultColumnFamily();

    size_t count = 0;
    rocksdb::ReadOptions readOptions;
    readOptions.total_order_seek = true;
    readOptions.tailing = true;
    std::unique_ptr<rocksdb::Iterator> iter(db()->NewIterator(readOptions, columnFamilyHandle));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) count++;
    return count;
  }

  void commitWriteBatch(rocksdb::WriteBatch* writeBatch) {
    rocksdb::Status status = db()->Write(rocksdb::WriteOptions(), writeBatch);
    CHECK(status.ok()) << "Commit write batch failed: " << status.ToString();
  }

 private:
  std::vector<std::string> columnFamilyNames_;
  RocksDbCfConfiguratorMap rocksDbCfConfiguratorMap_;
  RocksDbCfGroupConfigMap rocksDbCfGroupConfigMap_;
  rocksdb::DB* db_ = nullptr;
  std::shared_ptr<pipeline::DatabaseManager> databaseManager_;
  boost::filesystem::path dbPath_;
  pipeline::DatabaseManager::ColumnFamilyMap columnFamilyMap_;
  pipeline::DatabaseManager::ColumnFamilyGroupMap columnFamilyGroupMap_;
};

}  // namespace stesting

#endif  // STESTING_TESTWITHROCKSDB_H_
