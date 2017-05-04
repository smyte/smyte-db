# need to modify the code to inject the web server
genrule(
    name = "exposer_cc",
    srcs = [
        "lib/exposer.cc",
    ],
    outs = [
        "exposer.cc",
    ],
    cmd = "sed \"s/\#include \\\"handler/\#include \\\"lib\/handler/\" $< | " +
        "sed 's/Exposer(const std::string& bind_address)/Exposer(std::shared_ptr<CivetServer> server)/' | " +
        "sed  's/\: server_(new.*/\: server_(server),/' >$(@)",
)

genrule(
    name = "exposer_h",
    srcs = [
        "include/prometheus/exposer.h",
    ],
    outs = [
        "prometheus/exposer.h",
    ],
    cmd = "sed 's/\#include \\\"/\#include \\\"prometheus\//' $< | " +
        "sed 's/Exposer(const std::string& bind_address)/Exposer(std::shared_ptr<CivetServer> server)/' | " +
        "sed 's/std::unique_ptr<CivetServer>/std::shared_ptr<CivetServer>/' >$(@)",
)

cc_library(
    name = "prometheus",
    srcs = [
        ":exposer_cc",
        "lib/check_names.cc",
        "lib/counter.cc",
        "lib/counter_builder.cc",
        "lib/gauge.cc",
        "lib/gauge_builder.cc",
        "lib/handler.cc",
        "lib/handler.h",
        "lib/histogram.cc",
        "lib/histogram_builder.cc",
        "lib/json_serializer.cc",
        "lib/json_serializer.h",
        "lib/protobuf_delimited_serializer.cc",
        "lib/protobuf_delimited_serializer.h",
        "lib/registry.cc",
        "lib/serializer.h",
        "lib/text_serializer.cc",
        "lib/text_serializer.h",
    ],
    hdrs = [
        ":exposer_h",
    ],
    linkstatic = 1,
    deps = [
        ":prometheus_headers",
        "//external:civetweb",
        "//external:prometheus_client_model",
        "//external:protobuf",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "prometheus_headers",
    hdrs = [
        "include/prometheus/check_names.h",
        "include/prometheus/collectable.h",
        "include/prometheus/counter.h",
        "include/prometheus/counter_builder.h",
        "include/prometheus/family.h",
        "include/prometheus/gauge.h",
        "include/prometheus/gauge_builder.h",
        "include/prometheus/histogram.h",
        "include/prometheus/histogram_builder.h",
        "include/prometheus/metric.h",
        "include/prometheus/registry.h",
    ],
    strip_include_prefix = "include",
)
