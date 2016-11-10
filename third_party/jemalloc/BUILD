licenses(["notice"])

cc_library(
    name = "config",
    hdrs = glob([
        "jemalloc/*.h",
        "jemalloc/internal/*.h",
    ]),
    includes = [
        "jemalloc",
        "jemalloc/internal",
    ],
    visibility = ["//visibility:public"],
)
