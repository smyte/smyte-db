package(default_visibility = ["//visibility:public"])

cc_library(
    name = "serializer",
    srcs = [
        "Serializer.cpp",
    ],
    hdrs = [
        "Serializer.h",                                    
    ],
    deps = [
        "//external:glog",
        "//external:boost",
        "//external:rocksdb",

    ],
    copts = [
        "-std=c++14"
    ],
)
