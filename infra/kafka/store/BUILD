package(default_visibility = ["//visibility:public"])

cc_library(
    name = "consumer",
    srcs = [
        "Consumer.cpp",
    ],
    hdrs = [
        "Consumer.h",
        "KafkaStoreMessageRecord.hh",
    ],
    deps = [
        "//external:avro",
        "//external:folly",
        "//external:librdkafka",
        "//infra/kafka:abstract_consumer",
        "//infra/kafka:offset_manager",
        "//platform/gcloud:gcs",
    ],
)

cc_test(
    name = "consumer_test",
    srcs = [
        "ConsumerTest.cpp",
    ],
    size = "small",
    deps = [
        ":consumer",
        "//external:gtest_main",
    ],
)
