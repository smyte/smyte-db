licenses(["notice"])

# Modified from https://github.com/mzhaom/trunk/blob/master/third_party/wangle/BUILD

cc_library(
    name = "wangle",
    srcs = glob([
        "wangle/acceptor/*.cpp",
        "wangle/channel/*.cpp",
        "wangle/codec/*.cpp",
        "wangle/ssl/*.cpp",
        ],
        exclude = ["wangle/codec/CodecTest.cpp"],
    ) + [
        "wangle/bootstrap/ServerBootstrap.cpp",
    ],
    hdrs = glob([
        "wangle/acceptor/*.h",
        "wangle/bootstrap/*.h",
        "wangle/channel/*.h",
        "wangle/codec/*.h",
        "wangle/concurrent/*.h",
        "wangle/deprecated/rx/*.h",
        "wangle/ssl/*.h",
        "wangle/util/*.h",
    ]),
    includes = [
        ".",
    ],
    copts =[
        "-std=c++1y",
        "-Wno-sign-compare",
    ],
    linkopts = [
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        "//external:boost",
        "//external:boost_filesystem",
        "//external:boost_system",
        "//external:folly",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "concurrent_test",
    srcs = [
        "wangle/concurrent/test/CodelTest.cpp",
        "wangle/concurrent/test/GlobalExecutorTest.cpp",
        "wangle/concurrent/test/ThreadPoolExecutorTest.cpp",
    ],
    copts =[
        "-std=c++1y",
    ],
    deps = [
        ":wangle",
        "//external:gtest_main",
    ],
)
