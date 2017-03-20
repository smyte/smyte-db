licenses(["notice"])

cc_library(
    name = "storage",
    srcs = glob([
        "google/storage_api/*.cc",
    ]),
    hdrs = glob([
        "google/storage_api/*.h",
    ]),
    deps = [
        "//external:googleapis_curl_http",
        "//external:googleapis_http",
        "//external:googleapis_internal",
        "//external:googleapis_jsoncpp",
        "//external:googleapis_oauth2",
    ],
    visibility = ["//visibility:public"],
)
