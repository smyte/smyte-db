licenses(["notice"])

# copy the header to murmurhash3 directory to make cpplint happy
genrule(
    name = "exported_header",
    srcs = [
        "src/MurmurHash3.h",
    ],
    outs = [
        "murmurhash3/MurmurHash3.h",
    ],
    cmd = "cp $(<) $(@)",
)

cc_library(
    name = "murmurhash3",
    srcs = [
        "src/MurmurHash3.cpp",
        "src/MurmurHash3.h",
    ],
    hdrs = [
        ":exported_header",
    ],
    copts = [
        "-std=c++11",
        "-Wno-sign-compare",
    ],
    visibility = ["//visibility:public"],
)
