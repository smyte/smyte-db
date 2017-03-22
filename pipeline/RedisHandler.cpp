#include "pipeline/RedisHandler.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "boost/algorithm/string/case_conv.hpp"
#include "folly/Format.h"
#include "folly/String.h"
#include "glog/logging.h"
#include "pipeline/BuildVersion.h"
#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/statistics.h"
#include "rocksdb/table.h"

namespace pipeline {

void RedisHandler::read(Context* ctx, codec::RedisValue req) {
  if (req.type() == codec::RedisValue::Type::kError) {
    LOG(ERROR) << "Invalid request: " << req.error();
    write(ctx, req);
    return;
  }

  if (req.type() != codec::RedisValue::Type::kBulkStringArray) {
    LOG(ERROR) << "Invalid request: " << errorNotRedisArray().error();
    write(ctx, errorNotRedisArray());
    return;
  }

  const std::vector<std::string>& cmd = req.bulkStringArray();
  if (cmd.empty()) {
    LOG(ERROR) << "Empty request";
    return;
  }

  std::string cmdNameLower = boost::to_lower_copy(cmd.front());
  if (handleCommand(cmdNameLower, cmd, ctx)) {
    broadcastCmd(cmd, ctx);
  } else {
    writeError(folly::sformat("Unknown command: '{}'", cmdNameLower), ctx);
  }
}

codec::RedisValue RedisHandler::infoCommand(const std::vector<std::string>& cmd, Context* ctx) {
  std::stringstream ss;
  if (cmd.size() >= 2 && cmd[1] == "dbstats") {
    for (const auto& entry : databaseManager_->columnFamilyMap()) {
      std::string dbStats;
      db()->GetProperty(entry.second, "rocksdb.stats", &dbStats);
      ss << dbStats;
    }
  } else {
    appendToInfoOutput(&ss);
  }
  return { codec::RedisValue::Type::kBulkString, ss.str() };
}

void RedisHandler::appendToInfoOutput(std::stringstream* ss) {
  (*ss) << "# Server" << std::endl;
  (*ss) << "smyte_build_git_sha:" << kSmyteBuildGitSha << std::endl;
  (*ss) << "smyte_build_git_time:" << kSmyteBuildGitTime << std::endl;
  (*ss) << "smyte_build_compile_time:" << kSmyteBuildCompileTime << std::endl;
  (*ss) << std::endl;

  (*ss) << "# Clients" << std::endl;
  (*ss) << "connected_clients:" << getConnectionCount() << std::endl;
  (*ss) << std::endl;

  (*ss) << "# RocksDB" << std::endl;

  uint64_t value;
  // estimate live data size
  db()->GetIntProperty(rocksdb::DB::Properties::kEstimateLiveDataSize, &value);
  (*ss) << "estimate_live_data_size:" << value << std::endl;
  (*ss) << "estimate_live_data_size_human:" << (value >> 20) << 'M' << std::endl;

  // estimate number of keys
  db()->GetIntProperty(rocksdb::DB::Properties::kEstimateNumKeys, &value);
  (*ss) << "estimate_num_keys:" << value << std::endl;

  // memory usage
  uint64_t totalUsedMemory = 0;
  for (const auto& entry : databaseManager_->columnFamilyMap()) {
    rocksdb::ColumnFamilyHandle* columnFamily = entry.second;
    uint64_t usedMemory = 0;
    db()->GetIntProperty(columnFamily, rocksdb::DB::Properties::kEstimateTableReadersMem, &value);
    (*ss) << columnFamily->GetName() << "_cf_table_reader_memory:" << value << std::endl;
    (*ss) << columnFamily->GetName() << "_cf_table_reader_human:" << (value >> 20) << 'M' << std::endl;
    usedMemory += value;
    db()->GetIntProperty(columnFamily, rocksdb::DB::Properties::kSizeAllMemTables, &value);
    usedMemory += value;
    // block cache usage
    std::shared_ptr<rocksdb::TableFactory> tableFactory = db()->GetOptions(columnFamily).table_factory;
    if (strcmp(tableFactory->Name(), "BlockBasedTable") == 0) {
      rocksdb::BlockBasedTableOptions* tableOptions = static_cast<rocksdb::BlockBasedTableOptions*>(
          tableFactory->GetOptions());
      if (tableOptions->block_cache != nullptr) {
        usedMemory += tableOptions->block_cache->GetUsage();
      }
    }

    (*ss) << columnFamily->GetName() << "_cf_used_memory:" << usedMemory << std::endl;
    (*ss) << columnFamily->GetName() << "_cf_used_memory_human:" << (usedMemory >> 20) << 'M' << std::endl;

    totalUsedMemory += usedMemory;
  }

  (*ss) << "used_memory:" << totalUsedMemory << std::endl;
  (*ss) << "used_memory_human:" << (totalUsedMemory >> 20) << 'M' << std::endl;

  std::shared_ptr<rocksdb::Statistics> statistics = db()->GetOptions().statistics;
  // block cache hit ratio
  uint64_t blockCacheHit = statistics->getTickerCount(rocksdb::Tickers::BLOCK_CACHE_HIT);
  uint64_t blockCacheMiss = statistics->getTickerCount(rocksdb::Tickers::BLOCK_CACHE_MISS);
  (*ss) << "block_cache_hit_ratio:" << double(blockCacheHit) / (blockCacheHit + blockCacheMiss) << std::endl;

  rocksdb::HistogramData histData;
  // get time histogram
  statistics->histogramData(rocksdb::Histograms::DB_GET, &histData);
  outputStatistics("get", histData , ss);

  // write time histogram
  statistics->histogramData(rocksdb::Histograms::DB_WRITE, &histData);
  outputStatistics("write", histData, ss);

  // compaction time histogram
  statistics->histogramData(rocksdb::Histograms::COMPACTION_TIME, &histData);
  outputStatistics("compaction", histData, ss);
}

void RedisHandler::outputStatistics(const std::string& name, const rocksdb::HistogramData& histData,
                                    std::stringstream* ss) {
  (*ss) << "db_" << name << "_micros_median:" << histData.median << std::endl;
  (*ss) << "db_" << name << "_micros_average:" << histData.average << std::endl;
  (*ss) << "db_" << name << "_micros_percentile95:" << histData.percentile95 << std::endl;
  (*ss) << "db_" << name << "_micros_percentile99:" << histData.percentile99 << std::endl;
}

codec::RedisValue RedisHandler::monitorCommand(const std::vector<std::string>& cmd, Context* ctx) {
  bool alreadyMonitoring = false;
  {
    std::lock_guard<std::mutex> _guard(monitorMutex_);
    if (std::find(monitors_.begin(), monitors_.end(), ctx) == monitors_.end()) {
      monitors_.push_back(ctx);
    } else {
      alreadyMonitoring = true;
    }
  }

  if (alreadyMonitoring) {
    LOG(WARNING) << getPeerAddressPortStr(ctx) << " is already monitoring";
  }

  return simpleStringOk();
}

codec::RedisValue RedisHandler::freezeCommand(const std::vector<std::string>& cmd, Context* ctx) {
  std::vector<std::string> fileList;
  if (!databaseManager()->freeze(&fileList)) {
    return internalServerError();
  }

  return codec::RedisValue(std::move(fileList));
}

codec::RedisValue RedisHandler::thawCommand(const std::vector<std::string>& cmd, Context* ctx) {
  if (!databaseManager()->thaw()) {
    return internalServerError();
  }

  return simpleStringOk();
}

codec::RedisValue RedisHandler::compactCommand(const std::vector<std::string>& cmd, Context* ctx) {
  int args = cmd.size();
  std::string columnFamilyName = args > 1 ? cmd[1] : rocksdb::kDefaultColumnFamilyName;
  if (args == 3) {
    return errorResp("must specify begin and end keys");
  }

  std::string keyStart;
  std::string keyEnd;
  if (args == 4) {
    keyStart = cmd[2];
    keyEnd = cmd[3];
  }

  rocksdb::ColumnFamilyHandle* columnFamily = databaseManager()->getColumnFamily(columnFamilyName);
  if (!columnFamily) {
    return { codec::RedisValue::Type::kError, folly::sformat("Column family not found: {}", columnFamilyName) };
  }

  // compaction could take a very long time, don't block the I/O thread
  std::thread t([this, columnFamily, keyStart, keyEnd, args]() {
    if (args == 4) {
      rocksdb::Slice sBegin(keyStart);
      rocksdb::Slice sEnd(keyEnd);

      this->databaseManager()->forceCompaction(columnFamily, &sBegin, &sEnd);
    } else {
      this->databaseManager()->forceCompaction(columnFamily, nullptr, nullptr);
    }
  });
  // allow the worker thread to run in background
  t.detach();

  return simpleStringOk();
}

codec::RedisValue RedisHandler::pingCommand(const std::vector<std::string>& cmd, Context* ctx) {
  return { codec::RedisValue::Type::kSimpleString, "PONG" };
}

codec::RedisValue RedisHandler::selectCommand(const std::vector<std::string>& cmd, Context* ctx) {
  return simpleStringOk();
}

codec::RedisValue RedisHandler::getMetaCommand(const std::vector<std::string>& cmd, Context* ctx) {
  std::string value;
  rocksdb::Status status =
      db()->Get(rocksdb::ReadOptions(), databaseManager()->getMetadataColumnFamily(), cmd[1], &value);

  if (status.ok()) {
    return codec::RedisValue(codec::RedisValue::Type::kBulkString, std::move(value));
  }

  if (!status.IsNotFound()) {
    return errorResp(folly::sformat("RocksDB error: {}", status.ToString()));
  }

  return codec::RedisValue::nullString();
}

codec::RedisValue RedisHandler::setMetaCommand(const std::vector<std::string>& cmd, Context* ctx) {
  rocksdb::Status status =
      db()->Put(rocksdb::WriteOptions(), databaseManager()->getMetadataColumnFamily(), cmd[1], cmd[2]);

  if (status.ok()) {
    return simpleStringOk();
  }

  return errorResp(folly::sformat("RocksDB error: {}", status.ToString()));
}

void RedisHandler::broadcastCmd(const std::vector<std::string>& cmd, Context* ctx) {
  // quickly check if there is any pending monitors before the expensive locking
  if (UNLIKELY(!monitors_.empty())) {
    std::string monitorAddr = getPeerAddressPortStr(ctx);
    std::lock_guard<std::mutex> _guard(monitorMutex_);
    for (auto it = monitors_.cbegin(); it != monitors_.cend(); ++it) {
      Context* otherCtx = *it;
      if (ctx != otherCtx) {
        // due to wangle's async model, we cannot write to otherCtx directly but send it callback to it's thread
        otherCtx->getTransport()->getEventBase()->runInEventBaseThread([this, cmd, monitorAddr, otherCtx] {
          this->writeToMonitorContext(cmd, monitorAddr, otherCtx);
        });
      }
    }
  }
}

void RedisHandler::writeToMonitorContext(const std::vector<std::string>& cmd, const std::string& monitorAddr,
                                         Context* ctx) {
  std::lock_guard<std::mutex> _guard(monitorMutex_);
  if (std::find(monitors_.begin(), monitors_.end(), ctx) != monitors_.end()) {
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto seconds = now / 1000000;
    auto microseconds = now % 1000000;
    // format: 1458363281.367954 [0 172.17.42.1:55983] "get" "abc"
    std::string text = folly::sformat("{}.{:0>6} [0 {}] \"{}\"", seconds, microseconds, monitorAddr,
                                      folly::backslashify(folly::join("\" \"", cmd), true));
    write(ctx, { codec::RedisValue::Type::kSimpleString, std::move(text) });
  }
}

void RedisHandler::removeMonitor(Context* ctx) {
  std::lock_guard<std::mutex> _guard(monitorMutex_);
  auto it = std::find(monitors_.begin(), monitors_.end(), ctx);
  if (it != monitors_.end()) {
    LOG(INFO) << "monitoring by " << getPeerAddressPortStr(ctx) << " finished";
    monitors_.erase(it);
  }
}

bool RedisHandler::validateArgCount(const std::vector<std::string>& cmd, int minArgs, int maxArgs) {
  if (minArgs == -1 && maxArgs == -1) {
    // no check is necessary
    return true;
  }

  int numArgs = cmd.size() - 1;  // CMD + arguments
  if ((minArgs == -1 || numArgs >= minArgs) && (maxArgs == -1 || numArgs <= maxArgs)) {
    return true;
  }

  return false;
}

constexpr char RedisHandler::kWrongNumArgsTemplate[];

std::atomic<size_t> RedisHandler::connectionCount_;
std::vector<RedisHandler::Context*> RedisHandler::monitors_;
std::mutex RedisHandler::monitorMutex_;

}  // namespace pipeline
