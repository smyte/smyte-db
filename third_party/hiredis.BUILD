cc_library(
    name = "hiredis",
    srcs = [
        "dict.h",
        "fmacros.h",
        "hiredis.h",
        "net.h",
        "read.h",
        "sdsalloc.h",
        "sds.h",
        "dict.c",
        "hiredis.c",
        "net.c",
        "read.c",
        "sds.c",
    ],
    hdrs = [
        "hiredis.h",
        "net.h",
    ],
    include_prefix = "hiredis",
    copts = [
        "-Wno-unused-function"
    ],
    visibility = ["//visibility:public"],
)
