#include "pipeline/RedisPipelineBootstrap.h"

#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "folly/Conv.h"
#include "folly/Format.h"
#include "folly/init/Init.h"
#include "folly/json.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "infra/kafka/ConsumerHelper.h"
#include "infra/kafka/Producer.h"
#include "infra/ScheduledTaskQueue.h"
#include "librdkafka/rdkafkacpp.h"
#include "pipeline/KafkaConsumerConfig.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"
#include "wangle/bootstrap/ServerBootstrap.h"

// commandline flags shared by all services
// NOTE: any flags end with `_one_off` is a one-off flag that only gets applied once regardless of server restart.
// This is implemented by requiring a version timesstamp in milliseconds, which is compared to the version timestamp
// stored in rocksdb. Only one-off flags with a version newer than stored in db, but also not too old compared to the
// current timestamp will be applied.

DEFINE_int64(version_timestamp_ms, -1, "Version timestamp for the one-off flags");
// Indicates whether this replica is a master replica. Services may behave differently based on this value,
// though such behavior has to be coded explicitly. By default, the framework only uses this flag to decide where
// to run the backup script, which is external to the service code.
DEFINE_bool(master_replica, false, "Is this service replica the master replica");

// rocksdb settings
DEFINE_string(rocksdb_db_path, "/dev/null", "RocksDB data path");
DEFINE_int32(rocksdb_parallelism, std::thread::hardware_concurrency(), "Parallelism for flush and compaction");
DEFINE_int32(rocksdb_block_cache_size_mb, 512, "RocksDB block cache size in MB");
DEFINE_bool(rocksdb_create_if_missing_one_off, false, "Create database when missing");
// Convenience parameter to bootstrap the database without checking version_timestamp_ms
// NOTE: prefer the `_one_off` version in production
DEFINE_bool(rocksdb_create_if_missing, false, "Create database when missing without checking version_timestamp_ms");
// Create column families in groups for virtual sharding, e.g.,
// {
//    "node-to-smyte": {
//      "start_shard_index": 1,
//      "local_virtual_shard_count": 64,
//      "shard_index_increment": 16
//    }
///}
DEFINE_string(rocksdb_cf_group_configs, "{}", "RocksDB column family group configurations");
DEFINE_string(rocksdb_drop_cf_group_configs, "{}", "Same as rocksdb_cf_group_configs but specify the ones to drop");

// kafka flags
DEFINE_string(kafka_broker_list, "localhost:9092", "Kafka broker list");
// Example for kafka consumer configuration:
// [
//   {
//     "consumer_name": "EntityListKafkaConsumer",
//     "topic": "abcde-entityList",
//     "partition": 1,
//     "group_id": "entityList-b",
//     "offset_key_suffix": "day",
//     "consume_from_beginning_one_off": true,
//     "initial_offset_one_off": 1234,
//     "object_store_bucket_name": "kafka",
//     "object_store_object_name_prefix": "raw/",
//     "low_latency": true
//   }
// ]
// Note that topic, partition, and group_id are required. The rest are optional. By default, low_latency is disabled.
DEFINE_string(kafka_consumer_configs, "", "Kafka consumer configurations in JSON format");
// Example for kafka producer configuration:
// {
//   "entityList": {
//     "topic": abcde-entityList,
//     "partition": 1,
//     "low_latency": true
//   }
// }
// Note that the key for each entry is the topic name in production, while the 'topic' field inside a config entry
// is the full topic name that might contain prefix/suffix for testing purposes. By default, low_latency is disabled.
DEFINE_string(kafka_producer_configs, "", "Kafka producer configurations in JSON format");

// server settings
DEFINE_int32(port, 9049, "Server port");

// use a static global variable so that signal handlers can reference it
static std::shared_ptr<pipeline::RedisPipelineBootstrap> redisPipelineBootstrap;

namespace pipeline {

bool RedisPipelineBootstrap::canApplyOneOffFlags(int64_t versionTimestampMs) {
  if (versionTimestampMs < 0) {
    // version timestamp is not specified
    return false;
  }

  // first check if the timestamp is too old
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  if (now > versionTimestampMs) {
    LOG(WARNING) << "Specified version_timestamp_ms has already passed " <<
        "[" << (versionTimestampMs - now) << "milliseconds ago]";
    return false;
  }

  // check version timestamp stored in db
  if (rocksDb_ != nullptr) {
    std::string value;
    rocksdb::Status status =
        rocksDb_->Get(rocksdb::ReadOptions(), getColumnFamily(DatabaseManager::metadataColumnFamilyName()),
                      kVersionTimestampKey, &value);
    if (status.ok()) {
      int64_t storedTimestampMs = folly::to<int64_t>(value);
      if (storedTimestampMs >= versionTimestampMs) {
        LOG(WARNING) << "Specified version_timestamp_ms is older than db version";
        return false;
      }
      return true;
    }
    if (!status.IsNotFound()) {
      LOG(FATAL) << "Failed to load version timestamp from rocksdb: " << status.ToString();
    }
  }

  return true;
}

void RedisPipelineBootstrap::persistVersionTimestamp(int64_t versionTimestampMs) {
  if (canApplyOneOffFlags(versionTimestampMs)) {
    rocksdb::Status s =
        rocksDb_->Put(rocksdb::WriteOptions(), getColumnFamily(DatabaseManager::metadataColumnFamilyName()),
                      kVersionTimestampKey, folly::to<std::string>(versionTimestampMs));
    CHECK(s.ok()) << "Persisting version_timestamp_ms failed";
  }
}

void RedisPipelineBootstrap::processRocksDbColumnFamilyGroup(const std::string& groupName,
                                                             const RocksDbColumnFamilyGroupConfig& groupConfig,
                                                             std::function<void(const std::string&)> callback) {
  for (int i = 0; i < groupConfig.localVirtualShardCount; i++) {
    int shardNumber = groupConfig.startShardIndex + i * groupConfig.shardIndexIncrement;
    callback(getColumnFamilyNameInGroup(groupName, shardNumber));
  }
}

void RedisPipelineBootstrap::initializeRocksDb(const std::string& dbPath, const std::string& cfGroupConfigs,
                                               const std::string& dropCfGroupConfigs, int parallelism,
                                               int blockCacheSizeMb, bool createIfMissing, bool createIfMissingOneOff,
                                               int64_t versionTimestampMs) {
  rocksdb::Options options;
  // Optimize RocksDB
  // Common options for all types of workloads
  options.table_cache_numshardbits = 6;
  options.max_file_opening_threads = 16;
  options.wal_recovery_mode = rocksdb::WALRecoveryMode::kPointInTimeRecovery;
  options.write_buffer_size = 64 * 1024 * 1024;  // 64MB
  options.target_file_size_base = 64 * 1024 * 1024;  // 64MB
  options.max_bytes_for_level_base = 256 * 1024 * 1024;  // 256MB
  options.soft_pending_compaction_bytes_limit = 64L * 1024 * 1024 * 1024;  // 64GB
  options.hard_pending_compaction_bytes_limit = 256L * 1024 * 1024 * 1024;  // 256GB
  options.max_total_wal_size = 1024 * 1024 * 1024;  // 1GB
  options.IncreaseParallelism(parallelism);
  options.OptimizeLevelStyleCompaction();
  options.statistics = rocksdb::CreateDBStatistics();
  // this may hurt performance by helps bound memory usage
  // the expected sst file size is 64MB by default, so 1500 open files could address up to 96G data
  options.max_open_files = 1500;

  auto cfGroupConfigMap = parseRocksDbColumnFamilyGroupConfigs(cfGroupConfigs);
  auto dropCfGroupConfigMap = parseRocksDbColumnFamilyGroupConfigs(dropCfGroupConfigs);
  std::unordered_map<std::string, rocksdb::ColumnFamilyOptions> dropColumnFamilyOptionsMap;
  // Allow different services to customize column family configurations
  for (const auto& entry : config_.rocksDbConfiguratorMap) {
    rocksdb::ColumnFamilyOptions columnFamilyOptions(options);
    entry.second(blockCacheSizeMb, &columnFamilyOptions);
    const auto groupConfigIt = cfGroupConfigMap.find(entry.first);
    if (groupConfigIt == cfGroupConfigMap.end()) {
      // the configurator defines a single column family
      columnFamilyOptionsMap_[entry.first] = columnFamilyOptions;
    } else {
      // the configurator defines a column family group
      processRocksDbColumnFamilyGroup(entry.first, groupConfigIt->second, [&](const std::string& cfName) mutable {
        columnFamilyOptionsMap_[cfName] = columnFamilyOptions;
      });
    }
    // check column families to drop, we also need column family options for these in order to open db correctly
    const auto dropGroupConfigIt = dropCfGroupConfigMap.find(entry.first);
    if (dropGroupConfigIt != dropCfGroupConfigMap.end()) {
      processRocksDbColumnFamilyGroup(entry.first, dropGroupConfigIt->second, [&](const std::string& cfName) mutable {
        CHECK_EQ(columnFamilyOptionsMap_.count(cfName), 0) << "Cannot drop required column family: " << cfName;
        dropColumnFamilyOptionsMap[cfName] = columnFamilyOptions;
      });
    }
  }

  // optimize the required column families using point lookup when not specified by clients
  if (columnFamilyOptionsMap_.count(DatabaseManager::defaultColumnFamilyName()) == 0) {
    rocksdb::ColumnFamilyOptions columnFamilyOptions(options);
    columnFamilyOptions.OptimizeForPointLookup(blockCacheSizeMb);
    columnFamilyOptionsMap_[DatabaseManager::defaultColumnFamilyName()] = columnFamilyOptions;
  }
  if (columnFamilyOptionsMap_.count(DatabaseManager::metadataColumnFamilyName()) == 0) {
    rocksdb::ColumnFamilyOptions columnFamilyOptions(options);
    // smyte metadata is designed to store only a handful of keys, so 1MB cache suffice
    columnFamilyOptions.OptimizeForPointLookup(1);
    columnFamilyOptionsMap_[DatabaseManager::metadataColumnFamilyName()] = columnFamilyOptions;
  }

  optimizeBlockedBasedTable();

  struct stat buf;
  bool dbExists = (stat(folly::sformat("{}/CURRENT", dbPath).c_str(), &buf) == 0);
  if (!dbExists) {
    if (createIfMissing) {
      LOG(WARNING) << "Setting RocksDB option create_if_missing";
      options.create_if_missing = true;
    } else if (createIfMissingOneOff) {
      if (canApplyOneOffFlags(versionTimestampMs)) {
        LOG(WARNING) << "Setting RocksDB option create_if_missing as a one-off operation";
        options.create_if_missing = true;
      } else {
        LOG(WARNING) << "Cannot apply create_if_missing option unless a valid version_timestamp_ms is specified";
      }
    }
  }

  std::vector<std::string> existingColumnFamilies;
  std::vector<rocksdb::ColumnFamilyDescriptor> columnFamilyDescriptors;
  std::vector<rocksdb::ColumnFamilyHandle*> columnFamilyHandles;
  if (dbExists) {
    LOG(INFO) << "Loading existing database from " << dbPath;
    // check column families
    rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, dbPath, &existingColumnFamilies);
    CHECK(s.ok()) << "Listing column families failed: " << s.ToString();
  } else {
    LOG(INFO) << "Creating initial database in " << dbPath;
    // default column family is always created automatically when creating a new database
    existingColumnFamilies.emplace_back(DatabaseManager::defaultColumnFamilyName());
  }

  // prepare descriptors for existing column families
  for (const auto& name : existingColumnFamilies) {
    if (columnFamilyOptionsMap_.find(name) != columnFamilyOptionsMap_.end()) {
      // found a column family to open
      columnFamilyDescriptors.emplace_back(name, columnFamilyOptionsMap_[name]);
    } else if (dropColumnFamilyOptionsMap.find(name) != dropColumnFamilyOptionsMap.end()) {
      // found a column family to drop
      columnFamilyDescriptors.emplace_back(name, dropColumnFamilyOptionsMap[name]);
    } else {
      LOG(FATAL) << "Must define column family options for " << name;
    }
  }

  // open DB
  rocksdb::Status s = rocksdb::DB::Open(options, dbPath, columnFamilyDescriptors, &columnFamilyHandles, &rocksDb_);
  CHECK(s.ok()) << "RocksDB initialization failed: " << s.ToString();

  // Create missing column families
  for (const auto& entry : columnFamilyOptionsMap_) {
    auto it = find(existingColumnFamilies.cbegin(), existingColumnFamilies.cend(), entry.first);
    if (it == existingColumnFamilies.cend()) {
      rocksdb::ColumnFamilyHandle* cf;
      rocksdb::Status s = rocksDb_->CreateColumnFamily(columnFamilyOptionsMap_[entry.first], entry.first, &cf);
      CHECK(s.ok()) << "Creating column family `" << entry.first << "` failed: " << s.ToString();
      columnFamilyHandles.push_back(cf);
    }
  }

  // Map from name to column family pointer
  for (auto cf : columnFamilyHandles) {
    LOG(INFO) << "Loaded rocksdb column family: " << cf->GetName();
    columnFamilyMap_[cf->GetName()] = cf;
  }

  // Drop unused column families created by old column family group configs
  for (const auto& entry : dropColumnFamilyOptionsMap) {
    auto it = columnFamilyMap_.find(entry.first);
    if (it != columnFamilyMap_.end()) {
      LOG(INFO) << "Dropping column family: " << entry.first;
      rocksDb_->DropColumnFamily(it->second);
      rocksDb_->DestroyColumnFamilyHandle(it->second);
      columnFamilyMap_.erase(it);
    } else {
      LOG(ERROR) << "Column family to drop does not exist: " << entry.first;
    }
  }

  // Map from name to column family groups
  for (const auto& entry : cfGroupConfigMap) {
    std::vector<rocksdb::ColumnFamilyHandle*> group;
    processRocksDbColumnFamilyGroup(entry.first, entry.second, [&](const std::string& cfName) mutable {
      auto it = columnFamilyMap_.find(cfName);
      CHECK(it != columnFamilyMap_.end()) << "Column family not found: " << cfName;
      group.push_back(it->second);
    });
    columnFamilyGroupMap_[entry.first] = std::move(group);
  }

  // make sure the required column families are there
  CHECK_GT(columnFamilyMap_.count(DatabaseManager::defaultColumnFamilyName()), 0);
  CHECK_GT(columnFamilyMap_.count(DatabaseManager::metadataColumnFamilyName()), 0);
  for (const auto& entry : config_.rocksDbConfiguratorMap) {
    const auto groupConfigIt = cfGroupConfigMap.find(entry.first);
    if (groupConfigIt == cfGroupConfigMap.end()) {
      CHECK_GT(columnFamilyMap_.count(entry.first), 0);
    } else {
      processRocksDbColumnFamilyGroup(entry.first, groupConfigIt->second, [&](const std::string& cfName) mutable {
        CHECK_GT(columnFamilyMap_.count(cfName), 0);
      });
    }
  }
}

RedisPipelineBootstrap::RocksDbColumnFamilyGroupConfigMap RedisPipelineBootstrap::parseRocksDbColumnFamilyGroupConfigs(
    const std::string& configs) {
  folly::dynamic configJson = folly::dynamic::object;
  try {
    configJson = folly::parseJson(configs);
  } catch (const std::exception& e) {
    LOG(FATAL) << "rocksdb column family group configurations must be valid JSON: " << e.what();
  }
  RocksDbColumnFamilyGroupConfigMap configMap;
  for (const auto& entry : configJson.items()) {
    configMap.insert(std::make_pair(entry.first.getString(),
                                    RocksDbColumnFamilyGroupConfig(entry.second["start_shard_index"].getInt(),
                                                                   entry.second["local_virtual_shard_count"].getInt(),
                                                                   entry.second["shard_index_increment"].getInt())));
  }
  return configMap;
}

void RedisPipelineBootstrap::optimizeBlockedBasedTable() {
  for (const auto& entry : columnFamilyOptionsMap_) {
    std::shared_ptr<rocksdb::TableFactory> tableFactory = entry.second.table_factory;
    if (strcmp(tableFactory->Name(), "BlockBasedTable") == 0) {
      rocksdb::BlockBasedTableOptions* tableOptions = static_cast<rocksdb::BlockBasedTableOptions*>(
          tableFactory->GetOptions());
      // larger block size saves memory
      tableOptions->block_size = 32 * 1024;
    }
  }
}

void RedisPipelineBootstrap::initializeDatabaseManager(bool masterReplica) {
  CHECK_NOTNULL(rocksDb_);
  if (config_.databaseManagerFactory) {
    databaseManager_ = config_.databaseManagerFactory(columnFamilyMap_, masterReplica, rocksDb_, this);
  } else {
    databaseManager_ = std::make_shared<DatabaseManager>(columnFamilyMap_, masterReplica, rocksDb_);
  }
}

void RedisPipelineBootstrap::initializeKafkaProducers(const std::string& brokerList,
                                                      const std::string& kafkaProducerConfigs) {
  if (kafkaProducerConfigs.empty()) return;

  folly::dynamic configJson = folly::dynamic::object;
  try {
    configJson = folly::parseJson(kafkaProducerConfigs);
  } catch (const std::exception& e) {
    LOG(FATAL) << "--kafka_producer_configs must be valid JSON: " << e.what();
  }

  for (const auto& entry : configJson.items()) {
    infra::kafka::Producer::Config config;
    const folly::dynamic* partition = entry.second.get_ptr("partition");
    if (partition) config.partition = partition->getInt();
    const folly::dynamic* lowLatency = entry.second.get_ptr("low_latency");
    if (lowLatency) config.lowLatency = lowLatency->getBool();
    kafkaProducers_[entry.first.getString()] =
        std::make_shared<infra::kafka::Producer>(brokerList, entry.second["topic"].getString(), config);
  }
}

void RedisPipelineBootstrap::initializeScheduledTaskQueue() {
  if (config_.scheduledTaskProcessorFactory) {
    CHECK_NOTNULL(databaseManager_.get());
    rocksdb::ColumnFamilyHandle* columnFamily = getColumnFamily(infra::ScheduledTaskQueue::columnFamilyName());
    scheduledTaskQueue_ = std::make_shared<infra::ScheduledTaskQueue>(config_.scheduledTaskProcessorFactory(this),
                                                                      databaseManager_, columnFamily);
  }
}

void RedisPipelineBootstrap::initializeKafkaConsumer(const std::string& brokerList,
                                                     const std::string& kafkaConsumerConfigs,
                                                     int64_t versionTimestampMs) {
  if (config_.kafkaConsumerFactoryMap.empty() || kafkaConsumerConfigs.empty()) return;

  folly::dynamic configJson = folly::dynamic::object;
  try {
    configJson = folly::parseJson(kafkaConsumerConfigs);
  } catch (const std::exception& e) {
    LOG(FATAL) << "--kafka_consumer_configs must be valid JSON: " << e.what();
  }

  CHECK_NOTNULL(databaseManager_.get());
  if (config_.scheduledTaskProcessorFactory) {
    CHECK_NOTNULL(scheduledTaskQueue_.get());
  }

  kafkaConsumerHelper_ = std::make_shared<infra::kafka::ConsumerHelper>(
      rocksDb_, getColumnFamily(DatabaseManager::metadataColumnFamilyName()));

  for (const auto& configEntry : configJson) {
    KafkaConsumerConfig config = KafkaConsumerConfig::createFromJson(configEntry);
    KafkaConsumerFactory factory = config_.kafkaConsumerFactoryMap[config.consumerName];
    CHECK(factory) << "Kafka consumer factory for " << config.consumerName << " is not defined";

    const std::string offsetKey =
        kafkaConsumerHelper_->linkTopicPartition(config.topic, config.partition, config.offsetKeySuffix);
    if (config.consumeFromBeginningOneOff) {
      if (canApplyOneOffFlags(versionTimestampMs)) {
        LOG(WARNING) << "Consume partition " << config.partition << " of " << config.topic
                     << " from beginning as a one-off operation";
        CHECK(kafkaConsumerHelper_->commitRawOffset(offsetKey, RdKafka::Topic::OFFSET_BEGINNING));
      } else {
        LOG(WARNING) << "Cannot consume from beginning unless a valid version_timestamp_ms is specified";
      }
    } else if (config.initialOffsetOneOff >= 0) {
      if (canApplyOneOffFlags(versionTimestampMs)) {
        CHECK(kafkaConsumerHelper_->commitRawOffset(offsetKey, config.initialOffsetOneOff));
        LOG(WARNING) << "Consume partition " << config.partition << " of " << config.topic << " from "
                     << config.initialOffsetOneOff << " as a one-off operation";
      } else {
        LOG(WARNING) << "Cannot consume from the specified offset unless a valid version_timestamp_ms is specified";
      }
    }

    LOG(INFO) << "Launching kafka consumer for partition " << config.partition << " of " << config.topic << " as "
              << config.groupId;
    kafkaConsumers_.push_back(factory(brokerList, config, offsetKey, this));
  }
}

void RedisPipelineBootstrap::launchServer(int port) {
  LOG(INFO) << "Launching server on port " << port;
  server_ = new wangle::ServerBootstrap<RedisPipeline>();

  // Check the existence of dependencies based on configuration
  CHECK_NOTNULL(databaseManager_.get());
  if (config_.scheduledTaskProcessorFactory) {
    CHECK_NOTNULL(scheduledTaskQueue_.get());
  }

  server_->childPipeline(std::make_shared<pipeline::RedisPipelineFactory>(std::make_shared<DefaultRedisHandlerBuilder>(
      config_.redisHandlerFactory, config_.singletonRedisHandler, this)));

  server_->bind(port);
  server_->waitForStop();
  LOG(INFO) << "Pipeline server has shutdown gracefully";
}

std::shared_ptr<RedisPipelineBootstrap> RedisPipelineBootstrap::create(Config config) {
  redisPipelineBootstrap.reset(new RedisPipelineBootstrap(config));
  return redisPipelineBootstrap;
}

constexpr char RedisPipelineBootstrap::kVersionTimestampKey[];

}  // namespace pipeline

static void defaultSignalHandler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    redisPipelineBootstrap->stopServer();
  }
}

// Main function for any RedisPipeline-based service to include in their build
DECLARE_bool(logtostderr);
int main(int argc, char** argv) {
  FLAGS_logtostderr = true;  // good for docker, but it can be overwritten in runtime
  folly::init(&argc, &argv);

  signal(SIGINT, defaultSignalHandler);
  signal(SIGTERM, defaultSignalHandler);

  CHECK_EQ(pipeline::DatabaseManager::defaultColumnFamilyName(), rocksdb::kDefaultColumnFamilyName);

  LOG(INFO) << "Initializing RedisPipeline";
  redisPipelineBootstrap->initializeRocksDb(FLAGS_rocksdb_db_path, FLAGS_rocksdb_cf_group_configs,
                                            FLAGS_rocksdb_drop_cf_group_configs, FLAGS_rocksdb_parallelism,
                                            FLAGS_rocksdb_block_cache_size_mb, FLAGS_rocksdb_create_if_missing,
                                            FLAGS_rocksdb_create_if_missing_one_off, FLAGS_version_timestamp_ms);
  // initialize optional components
  // NOTE: order matters here because both the database manager and kafka producers maybe used by other components to
  // write data, so they should be initialized first
  redisPipelineBootstrap->initializeKafkaProducers(FLAGS_kafka_broker_list, FLAGS_kafka_producer_configs);
  redisPipelineBootstrap->initializeDatabaseManager(FLAGS_master_replica);
  redisPipelineBootstrap->initializeScheduledTaskQueue();
  redisPipelineBootstrap->initializeKafkaConsumer(FLAGS_kafka_broker_list, FLAGS_kafka_consumer_configs,
                                                  FLAGS_version_timestamp_ms);

  redisPipelineBootstrap->startOptionalComponents();

  redisPipelineBootstrap->persistVersionTimestamp(FLAGS_version_timestamp_ms);

  // start the server with all optional components initialized and started
  // NOTE: launchServer method cannot use any one-off flags
  redisPipelineBootstrap->launchServer(FLAGS_port);

  redisPipelineBootstrap->stopOptionalComponents();
  redisPipelineBootstrap->stopRocksDb();

  redisPipelineBootstrap.reset();
  return 0;
}
