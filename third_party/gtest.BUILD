licenses(["notice"])

# Modified from https://github.com/mzhaom/trunk/blob/master/third_party/gtest/BUILD

cc_library(
    name = "gtest",
    srcs = glob(["googletest/include/gtest/internal/**/*.h"]) + [
        "googletest/src/gtest-internal-inl.h",
        "googletest/src/gtest-death-test.cc",
        "googletest/src/gtest-filepath.cc",
        "googletest/src/gtest-port.cc",
        "googletest/src/gtest-printers.cc",
        "googletest/src/gtest-test-part.cc",
        "googletest/src/gtest-typed-test.cc",
        "googletest/src/gtest.cc",
    ],
    hdrs = [
        "googletest/include/gtest/gtest-death-test.h",
        "googletest/include/gtest/gtest-message.h",
        "googletest/include/gtest/gtest_pred_impl.h",
        "googletest/include/gtest/gtest_prod.h",
        "googletest/include/gtest/gtest-test-part.h",
        "googletest/include/gtest/gtest.h",
        "googletest/include/gtest/gtest-param-test.h",
        "googletest/include/gtest/gtest-printers.h",
        "googletest/include/gtest/gtest-spi.h",
        "googletest/include/gtest/gtest-typed-test.h"
    ],
    includes = [
        "googletest",
        "googletest/include",
    ],
    linkopts = [
        "-pthread"
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gtest_main",
    srcs = [
        "googletest/src/gtest_main.cc"
    ],
    linkopts = [
        "-pthread"
    ],
    deps = [
        ":gtest"
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gmock",
    hdrs = [
        "googlemock/include/gmock/gmock-actions.h",
        "googlemock/include/gmock/gmock-cardinalities.h",
        "googlemock/include/gmock/gmock-generated-actions.h",
        "googlemock/include/gmock/gmock-generated-function-mockers.h",
        "googlemock/include/gmock/gmock-generated-matchers.h",
        "googlemock/include/gmock/gmock-generated-nice-strict.h",
        "googlemock/include/gmock/gmock.h",
        "googlemock/include/gmock/gmock-matchers.h",
        "googlemock/include/gmock/gmock-more-actions.h",
        "googlemock/include/gmock/gmock-more-matchers.h",
        "googlemock/include/gmock/gmock-spec-builders.h",
    ],
    srcs = glob(["googlemock/include/gmock/internal/**/*.h"]) + [
        "googlemock/src/gmock-cardinalities.cc",
        "googlemock/src/gmock.cc",
        "googlemock/src/gmock-internal-utils.cc",
        "googlemock/src/gmock-matchers.cc",
        "googlemock/src/gmock-spec-builders.cc",
    ],
    deps = [
        ":gtest"
    ],
    includes = [
        "googlemock",
        "googlemock/include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gmock_main",
    srcs = [
        "googlemock/src/gmock_main.cc",
    ],
    linkopts = [
        "-pthread"
    ],
    deps = [
        ":gmock"
    ],
    visibility = ["//visibility:public"],
)
