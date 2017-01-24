package(default_visibility = ["//visibility:public"])

cc_library(
    name = "scheduled_task",
    hdrs = [
        "ScheduledTask.h",
    ],
    deps = [
        "//external:boost",
        "//external:glog",
        "//external:rocksdb",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_test(
    name = "scheduled_task_test",
    srcs = [
        "ScheduledTaskTest.cpp"
    ],
    size = "small",
    deps = [
        ":scheduled_task",
        "//external:gtest_main",
        "//external:rocksdb",
    ],
)

cc_library(
    name = "scheduled_task_processor",
    hdrs = [
        "ScheduledTaskProcessor.h",
    ],
    deps = [
        ":scheduled_task",
        "//external:glog",
        "//external:rocksdb",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_library(
    name = "scheduled_task_queue",
    srcs = [
        "ScheduledTaskQueue.cpp",
    ],
    hdrs = [
        "ScheduledTaskQueue.h",
    ],
    deps = [
        ":scheduled_task",
        ":scheduled_task_processor",
        "//external:glog",
        "//pipeline:database_manager",
        "//external:rocksdb",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_test(
    name = "scheduled_task_queue_test",
    srcs = [
        "ScheduledTaskQueueTest.cpp"
    ],
    size = "small",
    deps = [
        ":scheduled_task_queue",
        "//external:folly",
        "//external:gtest",
        "//external:gtest_main",
        "//stesting:test_helpers",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_library(
    name = "smyte_id",
    hdrs = [
        "SmyteId.h",
    ],
    deps = [
        "//external:boost",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_test(
    name = "smyte_id_test",
    srcs = [
        "SmyteIdTest.cpp",
    ],
    size = "small",
    deps = [
        ":smyte_id",
        "//external:gtest_main",
    ],
    copts = [
        "-std=c++14"
    ],
)

cc_library(
    name = "avro_helper",
    hdrs = [
        "AvroHelper.h"
    ],
    deps = [
        "//external:avro",
    ],
    copts = [
        "-std=c++14"
    ],
)
