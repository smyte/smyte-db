#ifndef STESTING_TESTWITHROCKSDB_H_
#define STESTING_TESTWITHROCKSDB_H_

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/filesystem.hpp"
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
  using RocksDbConfigurator = void (*)(int, rocksdb::ColumnFamilyOptions*);
  // Map column family names to RocksDbConfigurators
  using RocksDbConfiguratorMap = std::unordered_map<std::string, RocksDbConfigurator>;

  static void SetUpTestCase() {
    static bool initialized = false;
    if (!initialized) {
      FLAGS_logtostderr = true;
      gflags::InitGoogleLogging("stesting::TestWithRocksDb");
      gflags::InstallFailureSignalHandler();
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
                           RocksDbConfiguratorMap rocksDbConfiguratorMap = RocksDbConfiguratorMap())
      : columnFamilyNames_(std::move(optionalColumnFamilyNames)),
        rocksDbConfiguratorMap_(std::move(rocksDbConfiguratorMap)) {
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
    if (rocksDbConfiguratorMap_.count("default") > 0) {
      // 1MB default cache, it makes no difference for testing really
      rocksDbConfiguratorMap_["default"](1, &defaultColumnFamilyOptions);
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
      if (rocksDbConfiguratorMap_.count(name) > 0) {
        // 1MB default cache, it makes no difference for testing really
        rocksDbConfiguratorMap_[name](1, &columnFamilyOptions);
      }
      rocksdb::ColumnFamilyHandle* columnFamily;
      rocksdb::Status s = db_->CreateColumnFamily(columnFamilyOptions, name, &columnFamily);
      CHECK(s.ok()) << "Creating column family `" << name << "` failed: " << s.ToString();
      columnFamilyMap_[name] = columnFamily;
    }

    databaseManager_ = newDatabaseManager(columnFamilyMap_, db_);
  }

  void TearDown() override {
    for (auto& entry : columnFamilyMap_) {
      delete entry.second;
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
  RocksDbConfiguratorMap rocksDbConfiguratorMap_;
  rocksdb::DB* db_ = nullptr;
  std::shared_ptr<pipeline::DatabaseManager> databaseManager_;
  boost::filesystem::path dbPath_;
  pipeline::DatabaseManager::ColumnFamilyMap columnFamilyMap_;
};

}  // namespace stesting

#endif  // STESTING_TESTWITHROCKSDB_H_
