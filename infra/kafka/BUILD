package(default_visibility = ["//visibility:public"])

cc_library(
    name = "event_callback",
    srcs = [
        "EventCallback.cpp",
    ],
    hdrs = [
        "EventCallback.h",
    ],
    copts = [
        "-std=c++11",
    ],
    deps = [
        "//external:glog",
        "//external:librdkafka",
    ]
)

cc_library(
    name = "abstract_consumer",
    hdrs = [
        "AbstractConsumer.h",
    ],
    deps = [
        ":consumer_helper",
        "//external:glog",
    ],
)

cc_library(
    name = "consumer",
    srcs = [
        "Consumer.cpp",
    ],
    hdrs = [
        "Consumer.h",
    ],
    copts = [
        "-std=c++14",
    ],
    deps = [
        ":abstract_consumer",
        ":event_callback",
        "//external:folly",
        "//external:glog",
        "//external:librdkafka",
        "//external:snappy",
        "//external:zlib",
    ]
)

cc_test(
    name = "consumer_test",
    size = "small",
    srcs = [
        "ConsumerTest.h",
        "ConsumerTest.cpp",
    ],
    copts = [
        "-std=c++14",
    ],
    deps = [
        ":consumer",
        "//external:gflags",
        "//external:glog",
        "//external:gmock_main",
        "//external:gtest",
        "//external:librdkafka",
    ],
)

cc_library(
    name = "consumer_helper",
    srcs = [
        "ConsumerHelper.cpp",
    ],
    hdrs = [
        "ConsumerHelper.h",
    ],
    copts = [
        "-std=c++14",
    ],
    deps = [
        "//external:avro",
        "//external:folly",
        "//external:glog",
        "//external:rocksdb",
        "//external:librdkafka",
    ]
)

cc_test(
    name = "consumer_helper_test",
    size = "small",
    srcs = [
        "ConsumerHelperTest.cpp",
    ],
    deps = [
        ":consumer_helper",
        "//external:gtest_main",
        "//external:librdkafka",
        "//external:rocksdb",
        "//stesting:test_helpers",
    ],
)

cc_library(
    name = "producer",
    srcs = [
        "Producer.cpp",
    ],
    hdrs = [
        "Producer.h",
    ],
    deps = [
        ":event_callback",
        "//external:glog",
        "//external:librdkafka",
        "//external:snappy",
    ],
    copts = [
        "-std=c++11",
    ],
)

cc_library(
    name = "offset_manager",
    srcs = [
        "OffsetManager.cpp",
    ],
    hdrs = [
        "OffsetManager.h",
    ],
    deps = [
        "//external:folly",
        "//external:glog",
        "//external:librdkafka_c",
    ]
)
