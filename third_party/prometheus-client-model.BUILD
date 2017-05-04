genrule(
    name = "proto",
    srcs = [
        "metrics.proto",
    ],
    outs = [
        "metrics.pb.h",
        "metrics.pb.cc",
    ],
    tools = [
        "//external:protobuf_protoc"
    ],
    cmd = "cp $< $(@D); d=$$(pwd); cd $(@D); $$d/$(location //external:protobuf_protoc) metrics.proto --cpp_out=.",
)

cc_library(
    name = "client_model",
    srcs = [
        ":proto",
    ],
    hdrs = [
        "metrics.pb.h"
    ],
    deps = [
        "//external:protobuf",
    ],
    visibility = ["//visibility:public"],
)
