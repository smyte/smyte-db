#ifndef INFRA_SMYTEID_H_
#define INFRA_SMYTEID_H_

namespace infra {

class SmyteId {
 public:
  explicit SmyteId(int64_t smyteId) : smyteId_(smyteId) {}

  int getShardIndex(int shardCount) const {
    return (smyteId_ ^ (smyteId_ >> kMachineBits)) % shardCount;
  }

 private:
  static constexpr int kMachineBits = 13;

  int64_t smyteId_;
};

}  // namespace infra

#endif  // INFRA_SMYTEID_H_
