#ifndef PIPELINE_REDISPIPELINEBOOTSTRAP_H_
#define PIPELINE_REDISPIPELINEBOOTSTRAP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "infra/kafka/AbstractConsumer.h"
#include "infra/kafka/ConsumerHelper.h"
#include "infra/kafka/Producer.h"
#include "infra/ScheduledTaskProcessor.h"
#include "infra/ScheduledTaskQueue.h"
#include "librdkafka/rdkafkacpp.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "pipeline/DatabaseManager.h"
#include "pipeline/KafkaConsumerConfig.h"
#include "pipeline/RedisHandler.h"
#include "pipeline/RedisHandlerBuilder.h"
#include "pipeline/RedisPipelineFactory.h"
#include "wangle/bootstrap/ServerBootstrap.h"

namespace pipeline {

// RedisPipelineBootstrap is a template for launching RedisPipeline-based services in a main function
class RedisPipelineBootstrap {
 public:
  // Defines a list of optional components that a RedisPipeline may use
  struct OptionalComponents {
    std::shared_ptr<DatabaseManager> databaseManager;
    std::unordered_map<std::string, std::shared_ptr<infra::kafka::Producer>> kafkaProducers;
    std::shared_ptr<infra::ScheduledTaskQueue> scheduledTaskQueue;
    std::shared_ptr<infra::kafka::ConsumerHelper> kafkaConsumerHelper;
  };

  // A factory that create a RedisHandler with the given OptionalComponents
  using RedisHandlerFactory = std::shared_ptr<RedisHandler> (*)(const OptionalComponents&);

  // A factory that creates a kafka consumer
  using KafkaConsumerFactory = std::shared_ptr<infra::kafka::AbstractConsumer> (*)(
      const std::string&, const KafkaConsumerConfig&, const std::string&, std::shared_ptr<DatabaseManager>,
      std::shared_ptr<infra::kafka::ConsumerHelper>, std::shared_ptr<infra::ScheduledTaskQueue>);
  // Map kafka consumer config keys to consumer factories
  using KafkaConsumerFactoryMap = std::unordered_map<std::string, KafkaConsumerFactory>;

  // A factory that creates a database manager with a provided RocksDB column families and db instance
  using DatabaseManagerFactory = std::shared_ptr<DatabaseManager> (*)(
    const DatabaseManager::ColumnFamilyMap&, bool masterReplica, rocksdb::DB*);

  // A factory that creates a ScheduledTaskProcessor instance with a provided database manager
  using ScheduledTaskProcessorFactory = std::shared_ptr<infra::ScheduledTaskProcessor> (*)(
      std::shared_ptr<DatabaseManager>,
      std::unordered_map<std::string, std::shared_ptr<infra::kafka::Producer>>&);

  // Function to configure a column family in RocksDB, given a defaultBlockCacheSizeMb
  using RocksDbConfigurator = void (*)(int, rocksdb::ColumnFamilyOptions*);
  // Map column family names to RocksDbConfigurators
  using RocksDbConfiguratorMap = std::unordered_map<std::string, RocksDbConfigurator>;

  // A RedisHandlerBuilder that creates handler instances using the given factory method
  class DefaultRedisHandlerBuilder : public RedisHandlerBuilder {
   public:
    DefaultRedisHandlerBuilder(RedisHandlerFactory redisHandlerFactory, OptionalComponents optionalComponents,
                               bool singletonHandler)
        : redisHandlerFactory_(redisHandlerFactory),
          optionalComponents_(optionalComponents),
          singletonHandler_(singletonHandler) {
      if (singletonHandler) {
        // No race condition here since the constructor is only called in a single thread running bootstrap
        handler_ = redisHandlerFactory(optionalComponents);
        CHECK_NOTNULL(handler_.get());
      }
    }

    std::shared_ptr<RedisHandler> newHandler() override {
      if (singletonHandler_) {
        return handler_;
      } else {
        return redisHandlerFactory_(optionalComponents_);
      }
    }

   private:
    RedisHandlerFactory redisHandlerFactory_;
    OptionalComponents optionalComponents_;
    bool singletonHandler_;
    std::shared_ptr<RedisHandler> handler_;
  };

  // Defines function pointers to configure a RedisPipeline with optional components
  struct Config {
    // Required
    RedisHandlerFactory redisHandlerFactory = nullptr;

    // Optional
    KafkaConsumerFactoryMap kafkaConsumerFactoryMap;

    // Optional
    DatabaseManagerFactory databaseManagerFactory = nullptr;

    // Optional
    ScheduledTaskProcessorFactory scheduledTaskProcessorFactory = nullptr;

    // Optional
    // The default column family and smyte metadata column family are created and optimized for point lookups, but
    // their behaviors can be customized by providing corresponding RocksDbConfigurators. Additional column families
    // can be created based on the specification of this map. Note that it is not recommended to change the
    // configuration for smyte metadata.
    RocksDbConfiguratorMap rocksDbConfiguratorMap;

    // Optional
    // Indicate whether a singleton RedisHandler instance is sufficient for the pipeline
    // It is an optimization for the pipelines that do not save states to the handler instance
    // Most handlers should leave this optional true unless transaction support is need. See counters
    bool singletonRedisHandler = true;

    Config(RedisHandlerFactory _redisHandlerFactory,
           KafkaConsumerFactoryMap _kafkaConsumerFactoryMap = KafkaConsumerFactoryMap(),
           DatabaseManagerFactory _databaseManagerFactory = nullptr,
           ScheduledTaskProcessorFactory _scheduledTaskProcessorFactory = nullptr,
           RocksDbConfiguratorMap _rocksDbConfiguratorMap = RocksDbConfiguratorMap(),
           bool _singletonRedisHandler = true)
        : redisHandlerFactory(_redisHandlerFactory),
          kafkaConsumerFactoryMap(_kafkaConsumerFactoryMap),
          databaseManagerFactory(_databaseManagerFactory),
          scheduledTaskProcessorFactory(_scheduledTaskProcessorFactory),
          rocksDbConfiguratorMap(std::move(_rocksDbConfiguratorMap)),
          singletonRedisHandler(_singletonRedisHandler) {}
  };

  // Called by clients to create an instance to configure and start a server
  static std::shared_ptr<RedisPipelineBootstrap> create(Config config);

  // Create a kafka producer for the given topic
  static std::shared_ptr<infra::kafka::Producer> createKafkaProducer(
      std::string topic, infra::kafka::Producer::Config = infra::kafka::Producer::Config());

  ~RedisPipelineBootstrap() {}

  void initializeRocksDb(const std::string& dbPath, int parallelism, int blockCacheSizeMb, bool createIfMissing,
                         bool createIfMissingOneOff, int64_t versionMimestampMs);

  void stopRocksDb() {
    for (auto& entry : columnFamilyMap_) {
      delete entry.second;
    }
    delete rocksDb_;
    LOG(INFO) << "RocksDB has shutdown gracefully";
  }

  // optimize block-based table after all options are initialized
  void optimizeBlockedBasedTable();

  // Initialize optional components
  void initializeDatabaseManager(bool masterReplica);
  void initializeKafkaProducers(const std::string& brokerList, const std::string& kafkaProducerConfigs);
  void initializeKafkaConsumer(const std::string& brokerList, const std::string& kafkaConsumerConfigs,
                               int64_t versionTimestampMs);
  void initializeScheduledTaskQueue();

  void startOptionalComponents() {
    if (databaseManager_) {
      databaseManager_->start();
    }
    if (scheduledTaskQueue_) {
      scheduledTaskQueue_->start();
    }
    // First initialize all consumers then start their consumer loops
    // Initialization may panic on verification failures. Panic before starting any consumer loops reduces the
    // probability of data corruption since no writes can be committed until consumer loops start (if you don't use
    // kafka consumers then the following loops are no-op anyway).
    for (auto& consumer : kafkaConsumers_) {
      consumer->init(RdKafka::Topic::OFFSET_STORED);
    }
    for (auto& consumer : kafkaConsumers_) {
      consumer->start(1000);
    }
  }

  void stopOptionalComponents() {
    // stop in the reverse order of start
    for (auto& consumer : kafkaConsumers_) {
      consumer->destroy();
    }
    if (scheduledTaskQueue_) {
      scheduledTaskQueue_->destroy();
    }
    for (auto& producerEntry : kafkaProducers_) {
      producerEntry.second->destroy();
    }
    if (databaseManager_) {
      databaseManager_->destroy();
    }
  }

  // Create server and block on listening
  void launchServer(int port);

  // Stop server
  void stopServer() {
    if (server_) {
      server_->stop();
      server_ = nullptr;
    }
  }

  // Persist version timestamp to rocksdb
  void persistVersionTimestamp(int64_t versionTimestampMs);

  // Get the column family for the given name. Since we only call this during startup time, the program would terminate
  // if column family does not exist, in order to fail out loud.
  rocksdb::ColumnFamilyHandle* getColumnFamily(const std::string& name) {
    CHECK_GT(columnFamilyMap_.count(name), 0) << "Column family not found: " << name;
    return columnFamilyMap_[name];
  }

 private:
  static constexpr int64_t kMaxVersionTimestampAgeMs = 30 * 60 * 1000;  // 30 minutes
  static constexpr char kVersionTimestampKey[] = "VersionTimestamp";

  explicit RedisPipelineBootstrap(Config config) : config_(std::move(config)), rocksDb_(nullptr) {}

  // Validate if we can apply the one off flags
  bool canApplyOneOffFlags(int64_t versionTimestampMs);
  // Update ColumnFamilyOptions with block cache config for RocksDB
  void setRocksDbBlockCache(int blockCacheSizeMb, rocksdb::ColumnFamilyOptions* options);

  // Configurations for the RedisPipeline
  Config config_;

  // rocksdb pointers here are raw pointers since we want to deleted them explicitly for graceful shutdown
  rocksdb::DB* rocksDb_;
  DatabaseManager::ColumnFamilyMap columnFamilyMap_;
  std::unordered_map<std::string, rocksdb::ColumnFamilyOptions> columnFamilyOptionsMap_;

  // optional components
  std::shared_ptr<DatabaseManager> databaseManager_;
  std::shared_ptr<infra::ScheduledTaskQueue> scheduledTaskQueue_;
  std::shared_ptr<infra::kafka::ConsumerHelper> kafkaConsumerHelper_;
  // Store consumers as a vector because the same topic may be used by multiple consumer classes, and the same
  // consumer class may be used by different topics or the same topic with different configurations
  std::vector<std::shared_ptr<infra::kafka::AbstractConsumer>> kafkaConsumers_;
  // Producers are indexed by logical (canonical) topic names because of 1:1 mapping between topic and producer
  std::unordered_map<std::string, std::shared_ptr<infra::kafka::Producer>> kafkaProducers_;
  // require component
  // NOTE: use raw pointer here to avoid automatic deletion of the pointer.
  // server_->stop(); is sufficient for releasing resources
  wangle::ServerBootstrap<pipeline::RedisPipeline>* server_;
};

}  // namespace pipeline

#endif  // PIPELINE_REDISPIPELINEBOOTSTRAP_H_
