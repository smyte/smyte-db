licenses(["notice"])

# Modified from https://github.com/mzhaom/trunk/blob/master/third_party/folly/BUILD

genrule(
    name = "generate_format_tables",
    srcs = [
        "folly/build/generate_format_tables.py"
    ],
    outs = [
        "FormatTables.cpp",
    ],
    cmd = "$(location folly/build/generate_format_tables.py) "
        + "--install_dir=$(GENDIR)/external/" + REPOSITORY_NAME[1:],
)

genrule(
    name = "generate_escape_tables",
    srcs = [
        "folly/build/generate_escape_tables.py"
    ],
    outs = [
        "EscapeTables.cpp",
    ],
    cmd = "$(location folly/build/generate_escape_tables.py) "
        + "--install_dir=$(GENDIR)/external/" + REPOSITORY_NAME[1:],
)

genrule(
    name = "generate_varint_tables",
    srcs = [
        "folly/build/generate_varint_tables.py"
    ],
    outs = [
        "GroupVarintTables.cpp",
    ],
    cmd = "$(location folly/build/generate_varint_tables.py) "
        + "--install_dir=$(GENDIR)/external/" + REPOSITORY_NAME[1:],
)

cc_library(
    name = "folly",
    srcs = glob([
        "folly/fibers/*.cpp",
        "folly/futures/*.cpp",
        "folly/futures/detail/*.cpp",
    ]) + [
        ":generate_escape_tables",
        ":generate_format_tables",
        ":generate_varint_tables",
        "folly/Assume.cpp",
        "folly/Bits.cpp",
        "folly/Conv.cpp",
        "folly/Checksum.cpp",
        "folly/ClockGettimeWrappers.cpp",
        "folly/Demangle.cpp",
        "folly/File.cpp",
        "folly/FileUtil.cpp",
        "folly/Format.cpp",
        "folly/GroupVarint.cpp",
        "folly/IPAddress.cpp",
        "folly/IPAddressV4.cpp",
        "folly/IPAddressV6.cpp",
        "folly/LifoSem.cpp",
        "folly/MallctlHelper.cpp",
        "folly/MemoryMapping.cpp",
        "folly/MicroLock.cpp",
        "folly/Random.cpp",
        "folly/SafeAssert.cpp",
        "folly/SharedMutex.cpp",
        "folly/Shell.cpp",
        "folly/Singleton.cpp",
        "folly/SocketAddress.cpp",
        "folly/SpookyHashV1.cpp",
        "folly/SpookyHashV2.cpp",
        "folly/String.cpp",
        "folly/StringBase.cpp",
        "folly/Subprocess.cpp",
        "folly/ThreadCachedArena.cpp",
        "folly/TimeoutQueue.cpp",
        "folly/Unicode.cpp",
        "folly/Uri.cpp",
        "folly/Version.cpp",
        "folly/detail/CacheLocality.cpp",
        "folly/detail/Futex.cpp",
        "folly/detail/IPAddress.cpp",
        "folly/detail/MemoryIdler.cpp",
        "folly/detail/SocketFastOpen.cpp",
        "folly/detail/StaticSingletonManager.cpp",
        "folly/detail/ThreadLocalDetail.cpp",
        "folly/dynamic.cpp",
        "folly/experimental/AsymmetricMemoryBarrier.cpp",
        "folly/experimental/io/FsUtil.cpp",
        "folly/experimental/io/HugePages.cpp",
        "folly/experimental/observer/detail/Core.cpp",
        "folly/experimental/observer/detail/ObserverManager.cpp",
        "folly/io/Compression.cpp",
        "folly/io/Cursor.cpp",
        "folly/io/IOBuf.cpp",
        "folly/io/IOBufQueue.cpp",
        "folly/io/RecordIO.cpp",
        "folly/io/ShutdownSocketSet.cpp",
        "folly/io/async/AsyncSignalHandler.cpp",
        "folly/io/async/AsyncSocket.cpp",
        "folly/io/async/AsyncServerSocket.cpp",
        "folly/io/async/AsyncSSLSocket.cpp",
        "folly/io/async/AsyncUDPSocket.cpp",
        "folly/io/async/AsyncTimeout.cpp",
        "folly/io/async/EventBase.cpp",
        "folly/io/async/EventBaseManager.cpp",
        "folly/io/async/EventHandler.cpp",
        "folly/io/async/HHWheelTimer.cpp",
        "folly/io/async/Request.cpp",
        "folly/io/async/SSLContext.cpp",
        "folly/io/async/ssl/OpenSSLUtils.cpp",
        "folly/io/async/ssl/SSLErrors.cpp",
        "folly/json.cpp",
        "folly/memcpy.S",
        "folly/portability/BitsFunctexcept.cpp",
        "folly/portability/Dirent.cpp",
        "folly/portability/Environment.cpp",
        "folly/portability/Fcntl.cpp",
        "folly/portability/Libgen.cpp",
        "folly/portability/Malloc.cpp",
        "folly/portability/Memory.cpp",
        "folly/portability/Sockets.cpp",
        "folly/portability/Stdio.cpp",
        "folly/portability/Stdlib.cpp",
        "folly/portability/String.cpp",
        "folly/portability/SysFile.cpp",
        "folly/portability/SysMembarrier.cpp",
        "folly/portability/SysMman.cpp",
        "folly/portability/SysResource.cpp",
        "folly/portability/SysStat.cpp",
        "folly/portability/SysTime.cpp",
        "folly/portability/SysUio.cpp",
        "folly/portability/Time.cpp",
        "folly/portability/Unistd.cpp",
        "folly/ssl/OpenSSLHash.cpp",
        "folly/ssl/detail/SSLSessionImpl.cpp",
        "folly/stats/Instantiations.cpp",
    ],
    hdrs = glob([
        "folly/detail/*.h",
        "folly/experimental/observer/detail/*.h",
        "folly/futures/*.h",
        "folly/futures/detail/*.h",
        "folly/fibers/*.h",
        "folly/gen/*.h",
        "folly/io/async/*.h",
        "folly/io/async/ssl/*.h",
        "folly/ssl/detail/*.h",
        "folly/stats/*.h",
        "folly/*.h",
    ]) + [
        "folly/experimental/AsymmetricMemoryBarrier.h",
        "folly/experimental/ExecutionObserver.h",
        "folly/experimental/ReadMostlySharedPtr.h",
        "folly/experimental/TLRefCount.h",
        "folly/experimental/io/FsUtil.h",
        "folly/experimental/io/HugePages.h",
        "folly/io/Compression.h",
        "folly/io/Cursor.h",
        "folly/io/Cursor-inl.h",
        "folly/io/IOBuf.h",
        "folly/io/IOBufQueue.h",
        "folly/io/RecordIO.h",
        "folly/io/RecordIO-inl.h",
        "folly/io/ShutdownSocketSet.h",
        "folly/portability/Asm.h",
        "folly/portability/Atomic.h",
        "folly/portability/BitsFunctexcept.h",
        "folly/portability/Builtins.h",
        "folly/portability/Config.h",
        "folly/portability/Constexpr.h",
        "folly/portability/Dirent.h",
        "folly/portability/Environment.h",
        "folly/portability/Event.h",
        "folly/portability/Fcntl.h",
        "folly/portability/GFlags.h",
        "folly/portability/IOVec.h",
        "folly/portability/Libgen.h",
        "folly/portability/Malloc.h",
        "folly/portability/Math.h",
        "folly/portability/Memory.h",
        "folly/portability/PThread.h",
        "folly/portability/Sockets.h",
        "folly/portability/Stdio.h",
        "folly/portability/Stdlib.h",
        "folly/portability/String.h",
        "folly/portability/SysFile.h",
        "folly/portability/SysMembarrier.h",
        "folly/portability/SysMman.h",
        "folly/portability/SysResource.h",
        "folly/portability/SysStat.h",
        "folly/portability/SysSyscall.h",
        "folly/portability/SysTime.h",
        "folly/portability/SysTypes.h",
        "folly/portability/SysUio.h",
        "folly/portability/Time.h",
        "folly/portability/TypeTraits.h",
        "folly/portability/Windows.h",
        "folly/portability/Unistd.h",
        "folly/ssl/OpenSSLHash.h",
    ],
    includes = [
        ".",
    ],
    copts = [
        "-pthread",
        "-std=gnu++1y",
        "-Wno-unused-variable",
    ],
    linkopts = [
        "-ldl",
        "-lpthread",
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        "//external:boost",
        "//external:boost_context",
        "//external:boost_filesystem",
        "//external:boost_regex",
        "//external:folly_config",
        "//external:double_conversion",
        "//external:gflags",
        "//external:glog",
        "//external:jemalloc",
        "//external:libevent",
        "//external:snappy",
        "//external:zlib",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "AsciiCaseInsensitiveTest",
    size = "small",
    srcs = [
        "folly/test/AsciiCaseInsensitiveTest.cpp"
    ],
    deps = [
        ":folly",
        "//external:gflags",
        "//external:gtest",
    ]
)

