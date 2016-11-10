licenses(["notice"])

genrule(
    name = "googleapis_config",
    srcs = [
        "src/googleapis/config.h.in",
    ],
    outs = [
        "googleapis/config.h",
    ],
    cmd = "sed 's/@googleapis_VERSION_MAJOR@/0/g' $(<) | " +
        "sed 's/@googleapis_VERSION_MINOR@/2/g' | " +
        "sed 's/@googleapis_VERSION_PATCH@/0/g' | " +
        "sed 's/@googleapis_VERSION_DECORATOR@/devel/g' | " +
        "sed 's/#cmakedefine/\/\/#cmakedefine/g' | " +
        "sed 's/.*HAVE_OPENSSL.*/#define HAVE_OPENSSL 1/g '>$(@)",
)

cc_library(
    name = "internal",
    srcs = [
        ":googleapis_config",
        "src/googleapis/base/callback.cc",
        "src/googleapis/base/once.cc",
        "src/googleapis/base/strtoint.cc",
        "src/googleapis/strings/ascii_ctype.cc",
        "src/googleapis/strings/case.cc",
        "src/googleapis/strings/memutil.cc",
        "src/googleapis/strings/numbers.cc",
        "src/googleapis/strings/split.cc",
        "src/googleapis/strings/strcat.cc",
        "src/googleapis/strings/stringpiece.cc",
        "src/googleapis/strings/strip.cc",
        "src/googleapis/strings/util.cc",
        "src/googleapis/util/executor.cc",
        "src/googleapis/util/file.cc",
        "src/googleapis/util/hash.cc",
        "src/googleapis/util/status.cc",
    ],
    hdrs = [
        "src/googleapis/base/callback-specializations.h",
        "src/googleapis/base/callback-types.h",
        "src/googleapis/base/callback.h",
        "src/googleapis/base/integral_types.h",
        "src/googleapis/base/macros.h",
        "src/googleapis/base/mutex.h",
        "src/googleapis/base/once.h",
        "src/googleapis/base/port.h",
        "src/googleapis/base/strtoint.h",
        "src/googleapis/base/thread_annotations.h",
        "src/googleapis/client/util/status.h",
        "src/googleapis/strings/ascii_ctype.h",
        "src/googleapis/strings/case.h",
        "src/googleapis/strings/join.h",
        "src/googleapis/strings/memutil.h",
        "src/googleapis/strings/numbers.h",
        "src/googleapis/strings/split.h",
        "src/googleapis/strings/strcat.h",
        "src/googleapis/strings/stringpiece.h",
        "src/googleapis/strings/strip.h",
        "src/googleapis/strings/util.h",
        "src/googleapis/util/executor.h",
        "src/googleapis/util/file.h",
        "src/googleapis/util/hash.h",
        "src/googleapis/util/status.h",
    ],
    includes = [
        "src",
    ],
    copts = [
        "-Wno-sign-compare",
        "-Wno-unused-local-typedefs",
    ],
    deps = [
        "//external:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "utils",
    srcs = [
        "src/googleapis/client/data/base64_codec.cc",
        "src/googleapis/client/data/codec.cc",
        "src/googleapis/client/data/composite_data_reader.cc",
        "src/googleapis/client/data/data_reader.cc",
        "src/googleapis/client/data/data_writer.cc",
        "src/googleapis/client/data/file_data_reader.cc",
        "src/googleapis/client/data/inmemory_data_reader.cc",
        "src/googleapis/client/data/istream_data_reader.cc",
        "src/googleapis/client/util/date_time.cc",
        "src/googleapis/client/util/escaping.cc",
        "src/googleapis/client/util/file_utils.cc",
        "src/googleapis/client/util/program_path.cc",
        "src/googleapis/client/util/status.cc",
        "src/googleapis/client/util/uri_template.cc",
        "src/googleapis/client/util/uri_utils.cc",
    ],
    hdrs = [
        "src/googleapis/client/data/base64_codec.h",
        "src/googleapis/client/data/codec.h",
        "src/googleapis/client/data/data_reader.h",
        "src/googleapis/client/data/data_writer.h",
        "src/googleapis/client/util/date_time.h",
        "src/googleapis/client/util/escaping.h",
        "src/googleapis/client/util/file_utils.h",
        "src/googleapis/client/util/program_path.h",
        "src/googleapis/client/util/uri_template.h",
        "src/googleapis/client/util/uri_utils.h",
    ],
    copts = [
        "-Wno-sign-compare",
        "-Wno-unused-local-typedefs",
    ],
    deps = [
        ":internal",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "json",
    srcs = [
        "src/googleapis/client/data/serializable_json.cc",
    ],
    hdrs = [
        "src/googleapis/client/data/serializable_json.h",
    ],
    copts = [
        "-Wno-unused-local-typedefs",
    ],
    deps = [
        ":utils",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "http",
    srcs = [
        "src/googleapis/client/auth/credential_store.cc",
        "src/googleapis/client/auth/file_credential_store.cc",
        "src/googleapis/client/service/client_service.cc",
        "src/googleapis/client/service/media_uploader.cc",
        "src/googleapis/client/service/service_request_pager.cc",
        "src/googleapis/client/transport/ca_paths.cc",
        "src/googleapis/client/transport/http_authorization.cc",
        "src/googleapis/client/transport/http_request.cc",
        "src/googleapis/client/transport/http_request_batch.cc",
        "src/googleapis/client/transport/http_response.cc",
        "src/googleapis/client/transport/http_scribe.cc",
        "src/googleapis/client/transport/http_transport.cc",
        "src/googleapis/client/transport/http_transport_global_state.cc",
        "src/googleapis/client/transport/versioninfo.cc",
    ],
    hdrs = [
        "src/googleapis/client/auth/credential_store.h",
        "src/googleapis/client/auth/file_credential_store.h",
        "src/googleapis/client/service/client_service.h",
        "src/googleapis/client/service/media_uploader.h",
        "src/googleapis/client/service/service_request_pager.h",
        "src/googleapis/client/transport/ca_paths.h",
        "src/googleapis/client/transport/http_authorization.h",
        "src/googleapis/client/transport/http_request.h",
        "src/googleapis/client/transport/http_request_batch.h",
        "src/googleapis/client/transport/http_response.h",
        "src/googleapis/client/transport/http_scribe.h",
        "src/googleapis/client/transport/http_transport.h",
        "src/googleapis/client/transport/http_transport_global_state.h",
        "src/googleapis/client/transport/http_types.h",
        "src/googleapis/client/transport/versioninfo.h",
    ],
    copts = [
        "-Wno-sign-compare",
    ],
    deps = [
        ":internal",
        ":json",
        ":utils",
        "//external:curl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "curl_http",
    srcs = [
        "src/googleapis/client/transport/curl_http_transport.cc",
    ],
    hdrs = [
        "src/googleapis/client/transport/curl_http_transport.h",
    ],
    deps = [
        ":http",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "jsoncpp",
    srcs = [
        "src/googleapis/client/data/jsoncpp_data.cc",
    ],
    hdrs = [
        "src/googleapis/client/data/jsoncpp_data_helpers.h",
        "src/googleapis/client/data/jsoncpp_data.h",
    ],
    copts = [
        "-Wno-deprecated-declarations",
        "-Wno-unused-local-typedefs",
    ],
    deps = [
        ":json",
        ":utils",
        "//external:jsoncpp",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "oauth2",
    srcs = [
        "src/googleapis/client/auth/oauth2_authorization.cc",
    ],
    hdrs = [
        "src/googleapis/client/auth/oauth2_authorization.h",
    ],
    deps = [
        ":http",
        ":utils",
        "//external:jsoncpp",
    ],
    visibility = ["//visibility:public"],
)
