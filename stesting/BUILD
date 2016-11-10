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
        "//external:glog",
        "//external:gtest",
        "//external:rocksdb",
        "//pipeline:database_manager",
    ],
)
