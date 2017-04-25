cc_library(
    name = "wtf",
    hdrs = glob([
        "bindings/cpp/include/wtf/**/*.h",
    ]),
    srcs = [
        "bindings/cpp/buffer.cc",
        "bindings/cpp/event.cc",
        "bindings/cpp/platform.cc",
        "bindings/cpp/runtime.cc",
    ],
    copts = [
        "-std=c++11",
    ],
    strip_include_prefix = "bindings/cpp/include/",
    visibility = ["//visibility:public"],
)
