package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_helpers",
    hdrs = [
        "TestWithRocksDb.h"
    ],
    copts = [
        "-std=c++14",
    ],
    deps = [
        "//external:boost",
        "//external:folly",
        "//external:glog",
        "//external:gmock",
        "//external:rocksdb",
        "//pipeline:database_manager",
    ],
)
