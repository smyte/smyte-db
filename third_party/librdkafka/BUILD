licenses(["notice"])

cc_library(
    name = "config",
    hdrs = [
        "config.h",
        # use a placeholder file here to create the librdkafka subdirectory
        # so that source files can reference config using ../config.h
        "librdkafka/_placeholder_",
    ],
    includes = [
        "librdkafka",
    ],
    visibility = ["//visibility:public"],
)
