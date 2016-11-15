cc_binary(
    name = "key_value",
    srcs = [
        "KeyValue.cpp"
    ],
    deps = [
        "//external:folly",
        "//external:glog",
        "//external:rocksdb",
        "@smyte//codec:redis_value",
        "@smyte//pipeline:redis_handler",
        "@smyte//pipeline:redis_pipeline_bootstrap",
    ],
    copts = [
        "-std=c++14",
    ],
)
