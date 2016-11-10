#include "pipeline/DatabaseManager.h"

#include <memory>
#include <string>
#include <vector>

#include "folly/Format.h"
#include "glog/logging.h"
#include "rocksdb/transaction_log.h"

namespace pipeline {

bool DatabaseManager::freeze(std::vector<std::string>* fileList) {
  rocksdb::Status status;

  status = db_->DisableFileDeletions();
  if (!status.ok()) {
    LOG(ERROR) << "RocksDB DisableFileDeletions Error: " << status.ToString();
    return false;
  }

  // Fetch the live files without flushing the memtable
  std::vector<std::string> liveFiles;
  uint64_t manifestFileSize;
  status = db_->GetLiveFiles(liveFiles, &manifestFileSize, false);
  if (!status.ok()) {
    LOG(ERROR) << "RocksDB GetLiveFiles Error: " << status.ToString();
    return false;
  }

  // Fetch all of the write-ahead log files
  std::vector<std::unique_ptr<rocksdb::LogFile>> walLogs;
  status = db_->GetSortedWalFiles(walLogs);
  if (!status.ok()) {
    LOG(ERROR) << "RocksDB GetSortedWalFiles Error: " << status.ToString();
    return false;
  }

  fileList->reserve(fileList->size() + walLogs.size());

  std::string manifestPrefix = "/MANIFEST-";
  for (std::string liveFile : liveFiles) {
    if (std::equal(manifestPrefix.begin(), manifestPrefix.end(), liveFile.begin())) {
      fileList->emplace_back(folly::sformat("{}:{}", liveFile, manifestFileSize));
    } else {
      fileList->emplace_back(liveFile);
    }
  }
  for (auto& log : walLogs) {
    fileList->emplace_back(folly::sformat("{}:{}", log->PathName(), log->SizeFileBytes()));
  }

  return true;
}

}  // namespace pipeline
