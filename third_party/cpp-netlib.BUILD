licenses(["notice"])

# NOTE: this is an incomplete build. The purpose is to support the minimum functionality for embedded http server
cc_library(
    name = "cpp-netlib",
    hdrs = glob([
        "boost/**/*.hpp",
    ]),
    include_prefix = "cpp-netlib",
    deps = [
        ":cpp-netlib_impl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cpp-netlib_impl",
    srcs = glob([
        "libs/network/src/*.cpp",
    ]),
    hdrs = glob([
        "boost/**/*.hpp",
        "boost/**/*.ipp",
    ]),
    includes = [
        ".",
    ],
    deps = [
        ":uri",
        "//external:boost",
        "//external:boost_system",
    ],
)

cc_library(
    name = "uri",
    hdrs = glob([
        "deps/uri/include/**/*.hpp",
    ]),
    includes = [
        "deps/uri/include",
    ],
    deps = [
        "//external:boost",
    ],
)
