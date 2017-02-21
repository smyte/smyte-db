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

// This implementation is a modified version of folly::uriEscape. The main difference is that it also escapes `~`.
void DatabaseManager::escapeKeyStr(const std::string& str, std::string* out) {
  static const char hexValues[] = "0123456789ABCDEF";
  char esc[3];
  esc[0] = '%';
  // May need to escape 10% of the input string
  out->reserve(str.size() + out->size() + str.size() / 10);
  auto p = str.begin();
  auto last = p;
  while (p != str.end()) {
    char c = *p;
    unsigned char v = static_cast<unsigned char>(c);
    if (v < 33 || v > 125 || v == 37) {
      out->append(&*last, p - last);
      esc[1] = hexValues[v >> 4];
      esc[2] = hexValues[v & 0x0f];
      out->append(esc, 3);
      ++p;
      last = p;
    } else {
      // advance first and copy an entire block of normal characters at once
      ++p;
    }
  }
  out->append(&*last, p - last);
}

const DatabaseManager::ColumnFamilyGroupMap DatabaseManager::kEmptyColumnFamilyGroupMap = {};

}  // namespace pipeline
