cc_library(
    name = "civetweb",
    hdrs = [
        "include/civetweb.h",
        "include/CivetServer.h",
    ],
    strip_include_prefix = "include",
    include_prefix = "civetweb",
    deps = [
        ":civetweb_internal"
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "civetweb_internal",
    srcs = [
        "src/civetweb.c",
        "src/CivetServer.cpp",
    ],
    hdrs = [
        "include/civetweb.h",
        "include/CivetServer.h",
        "src/handle_form.inl",
        "src/md5.inl",
    ],
    includes = [
        "include",
    ],
    copts = [
        "-DUSE_IPV6",
        "-DNDEBUG",
        "-DNO_CGI",
        "-DNO_CACHING",
        "-DNO_SSL",
        "-DNO_FILES",
    ],
)
