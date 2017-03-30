package(default_visibility = ["//visibility:public"])

cc_library(
    name = "redis_pipeline_factory",
    hdrs = [
        "RedisPipelineFactory.h",
    ],
    deps = [
        ":redis_handler",
        ":redis_handler_builder",
        "//codec:redis_codec",
        "//codec:redis_value",
        "//external:folly",
        "//external:wangle",
    ],
    copts = [
        "-std=c++11",
    ],
)

cc_library(
    name = "redis_handler_builder",
    hdrs = [
        "RedisHandlerBuilder.h",
    ],
    deps = [
        ":redis_handler",
    ],
    copts = [
        "-std=c++11",
    ],
)

cc_library(
    name = "redis_handler",
    srcs = [
        "RedisHandler.cpp",
    ],
    hdrs = [
        "RedisHandler.h",
    ],
    deps = [
        ":build_version",
        ":database_manager",
        "//codec:redis_value",
        "//external:boost",
        "//external:folly",
        "//external:glog",
        "//external:rocksdb",
        "//external:wangle",
        "//infra/kafka:consumer_helper",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_test(
    name = "redis_handler_test",
    srcs = [
        "RedisHandlerTest.cpp",
    ],
    size = "small",
    deps = [
        ":redis_handler",
        "//external:gmock_main",
        "//external:gtest",
        "//stesting:test_helpers",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_library(
    name = "transactional_redis_handler",
    srcs = [
        "TransactionalRedisHandler.cpp",
    ],
    hdrs = [
        "TransactionalRedisHandler.h",
    ],
    deps = [
        ":redis_handler",
        "//codec:redis_value",
        "//external:boost",
        "//external:glog",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_library(
    name = "database_manager",
    srcs = [
        "DatabaseManager.cpp",
    ],
    hdrs = [
        "DatabaseManager.h",
    ],
    deps = [
        "//external:folly",
        "//external:glog",
        "//external:murmurhash3",
        "//external:rocksdb",
    ],
    copts = [
        "-std=c++11",
    ],
)

cc_test(
    name = "database_manager_test",
    size = "small",
    srcs = [
        "DatabaseManagerTest.cpp"
    ],
    deps = [
        ":database_manager",
        "//external:gtest_main",
    ],
    copts = [
        "-std=c++11",
    ],
)

cc_library(
    name = "redis_pipeline_bootstrap",
    srcs = [
        "RedisPipelineBootstrap.cpp",
    ],
    hdrs = [
        "RedisPipelineBootstrap.h",
    ],
    deps = [
        ":kafka_consumer_config",
        ":redis_handler",
        ":redis_handler_builder",
        ":redis_pipeline_factory",
        "//infra/kafka:abstract_consumer",
        "//infra/kafka:consumer_helper",
        "//infra/kafka:producer",
        "//infra:scheduled_task_queue",
        "//external:folly",
        "//external:gflags",
        "//external:glog",
        "//external:librdkafka",
        "//external:rocksdb",
        "//external:wangle",
    ],
    copts = [
        "-std=c++14"
    ]
)

cc_library(
    name = "kafka_consumer_config",
    srcs = [
        "KafkaConsumerConfig.cpp",
    ],
    hdrs = [
        "KafkaConsumerConfig.h",
    ],
    deps = [
        "//external:folly",
        "//external:librdkafka",
    ],
)

cc_test(
    name = "kafka_consumer_config_test",
    srcs = [
        "KafkaConsumerConfigTest.cpp",
    ],
    size = "small",
    deps = [
        ":kafka_consumer_config",
        "//external:folly",
        "//external:gtest_main",
        "//external:librdkafka",
    ],
)

cc_library(
    name = "build_version",
    srcs = [
        "BuildVersion.cpp",
    ],
    hdrs = [
        "BuildVersion.h",
    ],
    copts = [
        "-std=c++14"
    ],
)
