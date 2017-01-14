licenses(["notice"])

# Modified from https://github.com/mzhaom/trunk/blob/master/third_party/glog/BUILD

genrule(
    name = "glog_logging_h",
    srcs = [
        "src/glog/logging.h.in",
    ],
    outs = [
        "include/glog/logging.h",
    ],
    cmd = "sed 's/@ac_google_namespace@/google/g' $(<) | " +
        "sed 's/@ac_google_start_namespace@/namespace google {/g' | " +
        "sed 's/@ac_google_end_namespace@/}/g' | " +
        "sed 's/@ac_cv_have_unistd_h@/1/g' | " +
        "sed 's/@ac_cv_have_stdint_h@/1/g' | " +
        "sed 's/@ac_cv_have_systypes_h@/1/g' | " +
        "sed 's/@ac_cv_have_inttypes_h@/1/g' | " +
        "sed 's/@ac_cv_have_uint16_t@/1/g' | " +
        "sed 's/@ac_cv_have_u_int16_t@/1/g' | " +
        "sed 's/@ac_cv_have___builtin_expect@/1/g' | " +
        "sed 's/@ac_cv_have___uint16@/0/g' | " +
        "sed 's/@ac_cv_have_libgflags@/0/g' | " +
        "sed 's/@ac_cv___attribute___noreturn@/__attribute__ ((noreturn))/g' | " +
        "sed 's/@ac_cv___attribute___noinline@/__attribute__ ((noinline))/g' >$(@)",
)

genrule(
    name = "glog_log_severity_h",
    srcs = [
        "src/glog/log_severity.h",
    ],
    outs = [
        "include/glog/log_severity.h",
    ],
    cmd = "cp $(<) $(@)",
)

genrule(
    name = "glog_raw_logging_h",
    srcs = [
        "src/glog/raw_logging.h.in",
    ],
    outs = [
        "include/glog/raw_logging.h",
    ],
    cmd = "sed 's/@ac_google_namespace@/google/g' $(<) | " +
        "sed 's/@ac_google_start_namespace@/namespace google {/g' | " +
        "sed 's/@ac_google_end_namespace@/}/g' | " +
        "sed 's/@ac_cv___attribute___printf_4_5@/__attribute__((__format__ (__printf__, 4, 5)))/g' >$(@)",
)

genrule(
    name = "glog_stl_logging_h",
    srcs = [
        "src/glog/stl_logging.h.in",
    ],
    outs = [
        "include/glog/stl_logging.h",
    ],
    cmd = "sed 's/@ac_google_namespace@/google/g' $(<) | " +
        "sed 's/@ac_google_start_namespace@/namespace google {/g' | " +
        "sed 's/@ac_google_end_namespace@/}/g' | " +
        "sed 's/@ac_cv_cxx_using_operator@/1/g' >$(@)",
)

genrule(
    name = "glog_vlog_is_on_h",
    srcs = [
        "src/glog/vlog_is_on.h.in",
    ],
    outs = [
        "include/glog/vlog_is_on.h",
    ],
    cmd = "sed 's/@ac_google_namespace@/google/g' $(<) | " +
        "sed 's/@ac_google_start_namespace@/namespace google {/g' | " +
        "sed 's/@ac_google_end_namespace@/}/g' >$(@)",
)

cc_library(
    name = "glog",
    srcs = [
        "src/demangle.h",
        "src/symbolize.h",
        "src/utilities.h",
        "src/demangle.cc",
        "src/logging.cc",
        "src/raw_logging.cc",
        "src/signalhandler.cc",
        "src/symbolize.cc",
        "src/utilities.cc",
        "src/vlog_is_on.cc",
        "src/base/commandlineflags.h",
        "src/base/googleinit.h",
        "src/base/mutex.h",
        "src/stacktrace.h",
        "src/stacktrace_libunwind-inl.h",
    ],
    hdrs = [
        ":glog_logging_h",
        ":glog_log_severity_h",
        ":glog_raw_logging_h",
        ":glog_stl_logging_h",
        ":glog_vlog_is_on_h",
    ],
    defines = [
        "GOOGLE_GLOG_DLL_DECL=",
    ],
    includes = [
        "include",
    ],
    copts = [
        "-Wno-sign-compare",
    ],
    linkopts = [
        "-pthread"
    ],
    deps = [
        "//external:glog_config",
        "//external:libunwind",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "testlib",
    testonly = 1,
    hdrs = [
        "src/config_for_unittests.h",
        "src/googletest.h",
        "src/mock-log.h",
    ],
    data = [
        "src/demangle_unittest.txt",
        "src/logging_unittest.err",
    ],
    includes = [
        "src",
    ],
    deps = [
        "//external:gmock",
        "//external:gtest",
    ],
)

cc_test(
    name = "demangle_unittest",
    size = "small",
    srcs = [
        "src/demangle_unittest.cc",
    ],
    deps = [
        ":glog",
        ":testlib",
    ],
)

cc_test(
    name = "logging_unittest",
    size = "small",
    srcs = [
        "src/logging_unittest.cc",
    ],
    copts = [
        "-Wdeprecated-declarations",
    ],
    deps = [
        ":glog",
        ":testlib",
    ],
)

cc_test(
    name = "symbolize_unittest",
    size = "small",
    srcs = [
        "src/symbolize_unittest.cc",
    ],
    deps = [
        ":glog",
        ":testlib",
    ],
)
