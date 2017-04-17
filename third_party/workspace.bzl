# Programmatically define third-party dependencies so that external clients can load them
def smyte_workspace(workspace_name):
    if len(workspace_name) > 0 and not workspace_name.startswith("@"):
        workspace_name = "@" + workspace_name
    # avro
    native.new_http_archive(
        name = "avro_archive",
        url = "https://github.com/apache/avro/archive/release-1.8.1.tar.gz",
        strip_prefix = "avro-release-1.8.1",
        sha256 = "1bd406e161ca384c14a224cd12c9f6cb5f0b07924f7d52179f4e2691d1b91b50",
        build_file = workspace_name + "//third_party:avro.BUILD",
    )
    native.bind(
        name = "avro",
        actual = "@avro_archive//:avro",
    )
    native.bind(
        name = "avrogen",
        actual = "@avro_archive//:avrogen",
    )

    # boost
    native.new_http_archive(
        name = "boost_archive",
        url = "http://pilotfiber.dl.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.gz",
        sha256 = "fe34a4e119798e10b8cc9e565b3b0284e9fd3977ec8a1b19586ad1dec397088b",
        strip_prefix = "boost_1_63_0",
        build_file = workspace_name + "//third_party:boost.BUILD",
    )
    native.bind(
        name = "boost",
        actual = "@boost_archive//:boost",
    )
    native.bind(
        name = "boost_context",
        actual = "@boost_archive//:context",
    )
    native.bind(
        name = "boost_filesystem",
        actual = "@boost_archive//:filesystem",
    )
    native.bind(
        name = "boost_iostreams",
        actual = "@boost_archive//:iostreams",
    )
    native.bind(
        name = "boost_program_options",
        actual = "@boost_archive//:program_options",
    )
    native.bind(
        name = "boost_regex",
        actual = "@boost_archive//:regex",
    )
    native.bind(
        name = "boost_system",
        actual = "@boost_archive//:system",
    )

    # cpp-netlib
    native.new_git_repository(
        name = "cpp_netlib_git",
        remote = "https://github.com/cpp-netlib/cpp-netlib",
        commit = "f87285973b3d54801aedc356834e41729c3d0948",
        init_submodules = 1,
        build_file = workspace_name + "//third_party:cpp-netlib.BUILD",
    )
    native.bind(
        name = "cpp-netlib",
        actual = "@cpp_netlib_git//:cpp-netlib",
    )

    # curl
    native.new_http_archive(
        name = "curl_archive",
        url = "https://github.com/curl/curl/archive/curl-7_53_1.tar.gz",
        strip_prefix = "curl-curl-7_53_1",
        sha256 = "61806366d3e91c5671607560da3fc31f3f57ba021ecfd190f199048db2fdd5f6",
        build_file = workspace_name + "//third_party:curl.BUILD",
    )
    native.bind(
        name = "curl",
        actual = "@curl_archive//:curl",
    )
    native.bind(
        name = "curl_config",
        actual = workspace_name + "//third_party/curl:config"
    )

    # double-conversion
    native.new_http_archive(
        name = "double_conversion_archive",
        url = "https://github.com/google/double-conversion/archive/d4d68e4.tar.gz",
        strip_prefix = "double-conversion-d4d68e4e788bec89d55a6a3e33af674087837c82",
        sha256 = "4c21c78b4787051e682c7b5579ddb0c5fba823328faf14cdf52152a1f5bcdd32",
        build_file = workspace_name + "//third_party:double-conversion.BUILD",
    )
    native.bind(
        name = "double_conversion",
        actual = "@double_conversion_archive//:double-conversion",
    )

    # folly
    native.new_http_archive(
        name = "folly_archive",
        url = "https://github.com/facebook/folly/archive/v2017.04.17.00.tar.gz",
        strip_prefix = "folly-2017.04.17.00",
        sha256 = "a14d872cbf518369a607294e94d1068d27619051cf92277bf215b2536bf68f01",
        build_file = workspace_name + "//third_party:folly.BUILD",
    )
    native.bind(
        name = "folly",
        actual = "@folly_archive//:folly",
    )
    native.bind(
        name = "folly_config",
        actual = workspace_name + "//third_party/folly:config"
    )

    # gflags
    native.http_archive(
        name = "gflags_archive",
        url = "https://github.com/gflags/gflags/archive/f4eace1.tar.gz",
        strip_prefix = "gflags-f4eace133187e0a101a6d6d71c55592b572de189",
        sha256 = "4bc2b2d677aaeb26a319fc58096976bf162ccdae66db8bb50a98a925d62f71fa",
    )
    native.bind(
        name = "gflags",
        actual = "@gflags_archive//:gflags",
    )

    # glog
    native.new_http_archive(
        name = "glog_archive",
        url = "https://github.com/google/glog/archive/0472b91.tar.gz",
        strip_prefix = "glog-0472b91c5defdf90cff7292e3bf7bd86770a9a0a",
        sha256 = "abb174454241c1c5b84f83d256a4bf0def8c1e725e3c06650f361e51def4005e",
        build_file = workspace_name + "//third_party:glog.BUILD",
    )
    native.bind(
        name = "glog",
        actual = "@glog_archive//:glog",
    )
    native.bind(
        name = "glog_config",
        actual = workspace_name + "//third_party/glog:config"
    )

    # google cloud apis
    native.new_http_archive(
        name = "google_cloud_apis_archive",
        # Note: update the version in BUILD file
        url = "https://github.com/google/google-api-cpp-client/archive/bb5aed1.tar.gz",
        strip_prefix = "google-api-cpp-client-bb5aed14f3d9aa7f513aedf06a04a98c65e2972b",
        sha256 = "4f280cf21487161964556c2e0d61a3aff6429250929de1d9c94f92ff0265831e",
        build_file = workspace_name + "//third_party:google_cloud_apis.BUILD",
    )
    native.bind(
        name = "googleapis_http",
        actual = "@google_cloud_apis_archive//:http",
    )
    native.bind(
        name = "googleapis_curl_http",
        actual = "@google_cloud_apis_archive//:curl_http",
    )
    native.bind(
        name = "googleapis_internal",
        actual = "@google_cloud_apis_archive//:internal",
    )
    native.bind(
        name = "googleapis_jsoncpp",
        actual = "@google_cloud_apis_archive//:jsoncpp",
    )
    native.bind(
        name = "googleapis_oauth2",
        actual = "@google_cloud_apis_archive//:oauth2",
    )

    # google cloud storage
    native.new_http_archive(
        name = "google_cloud_storage_archive",
        # Note: update the version in BUILD file
        url = "https://developers.google.com/resources/api-libraries/download/storage/v1/cpp",
        type = "zip",
        strip_prefix = "storage",
        sha256 = "3b268d3dbc0b8d08c9004a18729e71244ca270209a9afab26ae70b22ede96213",
        build_file = workspace_name + "//third_party:google_cloud_storage.BUILD",
    )
    native.bind(
        name = "googleapis_storage",
        actual = "@google_cloud_storage_archive//:storage",
    )

    # gtest
    native.new_http_archive(
        name = "gtest_archive",
        url = "https://github.com/google/googletest/archive/release-1.8.0.tar.gz",
        strip_prefix = "googletest-release-1.8.0",
        sha256 = "58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8",
        build_file = workspace_name + "//third_party:gtest.BUILD",
    )
    native.bind(
        name = "gtest",
        actual = "@gtest_archive//:gtest",
    )
    native.bind(
        name = "gtest_main",
        actual = "@gtest_archive//:gtest_main",
    )
    native.bind(
        name = "gmock",
        actual = "@gtest_archive//:gmock",
    )
    native.bind(
        name = "gmock_main",
        actual = "@gtest_archive//:gmock_main",
    )

    # jsoncpp
    native.new_http_archive(
        name = "jsoncpp_archive",
        url = "https://github.com/open-source-parsers/jsoncpp/archive/1.7.7.tar.gz",
        strip_prefix = "jsoncpp-1.7.7",
        sha256 = "087640ebcf7fbcfe8e2717a0b9528fff89c52fcf69fa2a18cc2b538008098f97",
        build_file = workspace_name + "//third_party:jsoncpp.BUILD",
    )
    native.bind(
        name = "jsoncpp",
        actual = "@jsoncpp_archive//:jsoncpp",
    )

    # jemalloc
    native.new_http_archive(
        name = "jemalloc_archive",
        url = "https://github.com/jemalloc/jemalloc/archive/4.2.1.tar.gz",
        strip_prefix = "jemalloc-4.2.1",
        sha256 = "38abd5c3798dee4bd0e63e082502358cd341b831b038bb443e89370df888a3eb",
        build_file = workspace_name + "//third_party:jemalloc.BUILD",
    )
    native.bind(
        name = "jemalloc",
        actual = "@jemalloc_archive//:jemalloc",
    )
    native.bind(
        name = "jemalloc_config",
        actual = workspace_name + "//third_party/jemalloc:config"
    )

    # libevent
    native.new_http_archive(
        name = "libevent_archive",
        url = "https://github.com/libevent/libevent/archive/release-2.0.22-stable.tar.gz",
        strip_prefix = "libevent-release-2.0.22-stable",
        sha256 = "ab89639b0819befb1d8b293d52047c6955f8d1c9150c2b22a0e6247930eb9128",
        build_file = workspace_name + "//third_party:libevent.BUILD",
    )
    native.bind(
        name = "libevent",
        actual = "@libevent_archive//:libevent",
    )
    native.bind(
        name = "libevent_config",
        actual = workspace_name + "//third_party/libevent:config"
    )

    # librdkafka
    native.new_git_repository(
        name = "librdkafka_git",
        remote = "https://github.com/edenhill/librdkafka",
        tag = "v0.9.4",
        build_file = workspace_name + "//third_party:librdkafka.BUILD",
    )
    native.bind(
        name = "librdkafka_c",
        actual = "@librdkafka_git//:librdkafka_c",
    )
    native.bind(
        name = "librdkafka",
        actual = "@librdkafka_git//:librdkafka",
    )
    native.bind(
        name = "librdkafka_config",
        actual = workspace_name + "//third_party/librdkafka:config"
    )

    # libunwind
    native.new_git_repository(
        name = "libunwind_git",
        remote = "git://git.sv.gnu.org/libunwind.git",
        tag = "v1.1", # Note: update the version in BUILD file
        build_file = workspace_name + "//third_party:libunwind.BUILD",
    )
    native.bind(
        name = "libunwind",
        actual = "@libunwind_git//:libunwind",
    )
    native.bind(
        name = "libunwind_config",
        actual = workspace_name + "//third_party/libunwind:config"
    )

    # murmurhash3
    native.new_http_archive(
        name = "murmurhash3_archive",
        url = "https://github.com/aappleby/smhasher/archive/61a0530.tar.gz",
        strip_prefix = "smhasher-61a0530f28277f2e850bfc39600ce61d02b518de",
        sha256 = "daa4bb23e24fe26a2f9d1bb0d241fff7eb963f7b7525fa20685e0dfb3b3ffa49",
        build_file = workspace_name + "//third_party:murmurhash3.BUILD",
    )
    native.bind(
        name = "murmurhash3",
        actual = "@murmurhash3_archive//:murmurhash3",
    )

    #re2
    native.git_repository(
        name = "com_googlesource_code_re2",  # match the name defined in its WORKSPACE file
        remote = "https://github.com/google/re2.git",
        tag = "2017-01-01",
    )
    native.bind(
        name = "re2",
        actual = "@com_googlesource_code_re2//:re2",
    )

    # rocksdb
    native.new_git_repository(
        name = "rocksdb_git",
        remote = "https://github.com/facebook/rocksdb.git",
        tag = "v5.2.1",
        build_file = workspace_name + "//third_party:rocksdb.BUILD",
    )
    native.bind(
        name = "rocksdb",
        actual = "@rocksdb_git//:rocksdb",
    )

    # snappy
    native.new_http_archive(
        name = "snappy_archive",
        url = "https://github.com/google/snappy/archive/ed3b7b2.tar.gz",
        strip_prefix = "snappy-ed3b7b242bd24de2ca6750c73f64bee5b7505944",
        sha256 = "88a644b224f54edcd57d01074c2d6fd6858888e915c21344b8622c133c35a337",
        build_file = workspace_name + "//third_party:snappy.BUILD",
    )
    native.bind(
        name = "snappy",
        actual = "@snappy_archive//:snappy",
    )
    native.bind(
        name = "snappy_config",
        actual = workspace_name + "//third_party/snappy:config"
    )

    # wangle
    native.new_http_archive(
        name = "wangle_archive",
        url = "https://github.com/facebook/wangle/archive/v2017.04.17.00.tar.gz",
        strip_prefix = "wangle-2017.04.17.00",
        sha256 = "10dd53b6104f96d4c97fb39072aeb2b42cbb2a2ec91d047ae11e5596654ca324",
        build_file = workspace_name + "//third_party:wangle.BUILD",
    )
    native.bind(
        name = "wangle",
        actual = "@wangle_archive//:wangle",
    )

    # zlib
    native.new_http_archive(
        name = "zlib_archive",
        url = "https://github.com/madler/zlib/archive/v1.2.8.tar.gz",
        strip_prefix = "zlib-1.2.8",
        sha256 = "e380bd1bdb6447508beaa50efc653fe45f4edc1dafe11a251ae093e0ee97db9a",
        build_file = workspace_name + "//third_party:zlib.BUILD",
    )
    native.bind(
        name = "zlib",
        actual = "@zlib_archive//:zlib",
    )
