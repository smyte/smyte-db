# Description:
# Codecs for Smyte's supported protocols

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "redis_value",
    srcs = [
        "RedisValue.cpp",
    ],
    hdrs = [
        "RedisValue.h",
    ],
    deps = [
        "//external:boost",
        "//external:glog",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_test(
    name = "redis_value_test",
    srcs = [
        "RedisValueTest.cpp",
    ],
    size = "small",
    deps = [
        ":redis_value",
        "//external:gtest_main",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_library(
    name = "redis_message",
    hdrs = [
        "RedisMessage.h",
    ],
    deps = [
        ":redis_value",
    ],
    copts = [
        "-std=c++14",
    ],
)

cc_library(
    name = "redis_codec",
    srcs = [
        "RedisDecoder.cpp",
    ],
    hdrs = [
        "RedisEncoder.h",
        "RedisDecoder.h",
    ],
    copts = [
        "-std=c++14",
    ],
    deps = [
        ":redis_message",
        "//external:folly",
        "//external:glog",
        "//external:wangle",
    ],
)

cc_test(
    name = "redis_codec_test",
    srcs = [
        "RedisCodecTest.cpp",
    ],
    size = "small",
    deps = [
        ":redis_codec",
        ":redis_value",
        "//external:folly",
        "//external:gtest_main",
    ],
    copts = [
        "-std=c++14",
    ],
)
