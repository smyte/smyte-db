cc_library(
    name = "hiredis_c",
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
    include_prefix = "hiredis_c",
    copts = [
        "-Wno-unused-function"
    ],
    visibility = ["//visibility:public"],
)
