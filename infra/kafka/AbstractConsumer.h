#ifndef INFRA_KAFKA_ABSTRACTCONSUMER_H_
#define INFRA_KAFKA_ABSTRACTCONSUMER_H_

#include <memory>
#include <chrono>
#include <string>
#include <thread>

#include "glog/logging.h"
#include "infra/kafka/ConsumerHelper.h"

namespace infra {
namespace kafka {

// Consumer interface for both standard kafka consumer and kafka-store consumer
class AbstractConsumer {
 public:
  static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  AbstractConsumer(const std::string& offsetKey, std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper)
      : offsetKey_(offsetKey),
        consumerHelper_(consumerHelper),
        initialized_(false),
        // run_ defaults to true so that client can signal to stop the consumer thread when it's about to start
        run_(true),
        consumerThread_(nullptr) {}

  virtual ~AbstractConsumer() {}

  // Initialize the consumer with verifications for topics, partitions, etc.
  // Should set initialized_ to true once its done.
  // Should panic for any failed verification.
  virtual void init(int64_t initialOffset) = 0;

  // Start the consumer with initial offset and read timeout.
  // Should NOT panic unless encountered a programming error.
  virtual void start(int timeoutMs) {
    CHECK(initialized()) << "Consumer has not been initialized";
    // prevent reusing the consumer object
    CHECK(consumerThread_ == nullptr) << "Kafka store consumer thread already started";

    // `this` pointer has a longer lifetime than the consumer thread, so it's okay just pass `this` to the thread
    consumerThread_.reset(new std::thread([this, timeoutMs]() {
      while (this->run()) {
        // process a batch of messages
        this->processBatch(timeoutMs);
      }
    }));
  }

  // Stop the consumer. This function should NOT block.
  virtual void stop(void) {
    run_ = false;
  }

  // Wait for consumer to stop. This function may block.
  virtual void waitForStop(void) {
    CHECK(consumerThread_ != nullptr) << "Kafka consumer thread has not been created";

    if (consumerThread_ && consumerThread_->joinable()) {
      consumerThread_->join();
    }
  }

  // Stop the consumer and release resources.
  virtual void destroy(void) = 0;

  // Load committed offset from persistent storage.
  virtual int64_t loadCommittedKafkaOffset(void) = 0;

  // Process a batch of messages within the given timeout.
  // Note that we don't define processOne function here as consumers may use different message types
  virtual void processBatch(int timeoutMs) = 0;

 protected:
  const std::string& offsetKey() const { return offsetKey_; }
  std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper() { return consumerHelper_; }
  bool run() const { return run_; }
  bool initialized() const { return initialized_; }
  void setInitialized() { initialized_ = true; }

 private:
  const std::string offsetKey_;
  std::shared_ptr<infra::kafka::ConsumerHelper> consumerHelper_;
  bool initialized_;
  bool run_;
  std::unique_ptr<std::thread> consumerThread_;
};

}  // namespace kafka
}  // namespace infra

#endif  // INFRA_KAFKA_ABSTRACTCONSUMER_H_
