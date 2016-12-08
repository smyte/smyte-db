#ifndef INFRA_SMYTEID_H_
#define INFRA_SMYTEID_H_

#include <string>

#include "boost/endian/buffers.hpp"

namespace infra {

class SmyteId {
 public:
  explicit SmyteId(int64_t smyteId) : smyteId_(smyteId) {}

  // Append the smyte id to the output as an eight byte string. Use big endian so that it can be sorted as string.
  void appendAsBinary(std::string* out) const {
    boost::endian::big_int64_buf_t value(smyteId_);
    out->append(value.data(), sizeof(int64_t));
  }

  int getShardIndex(int shardCount) const {
    return (smyteId_ ^ (smyteId_ >> kMachineBits)) % shardCount;
  }

 private:
  static constexpr int kMachineBits = 13;

  int64_t smyteId_;
};

}  // namespace infra

#endif  // INFRA_SMYTEID_H_
