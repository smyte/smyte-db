licenses(["notice"])

genrule(
    name = "internal_jemalloc_internal_h",
    srcs = [
        "include/jemalloc/internal/jemalloc_internal.h.in",
    ],
    outs = [
        "include/jemalloc/internal/jemalloc_internal.h",
    ],
    cmd = "sed 's/@install_suffix@//g' $(<) | " +
        "sed 's/@private_namespace@/je/g' >$(@)",
)

genrule(
    name = "internal_private_namespace_h",
    srcs = [
        "include/jemalloc/internal/private_namespace.sh",
        "include/jemalloc/internal/private_symbols.txt",
    ],
    outs = [
        "include/jemalloc/internal/private_namespace.h",
    ],
    cmd = "$(locations include/jemalloc/internal/private_namespace.sh) " +
        "$(locations include/jemalloc/internal/private_symbols.txt) >$(@)"
)

# size classes
LG_QUANTA = "3 4"
LG_TINY_MIN = "3"
LG_PAGE_SIZES = "12"
LG_SIZE_CLASS_GROUP = "2"
SIZE_CLASSES = [LG_QUANTA, LG_TINY_MIN, LG_PAGE_SIZES, LG_SIZE_CLASS_GROUP]
genrule(
    name = "internal_size_classes_h",
    srcs = [
        "include/jemalloc/internal/size_classes.sh",
    ],
    outs = [
        "include/jemalloc/internal/size_classes.h",
    ],
    cmd = "$(<) \"" + "\" \"".join(SIZE_CLASSES) + "\">$(@)",
)

LG_SIZEOF_PTR = "3"
genrule(
    name = "jemalloc_defs_h",
    srcs = [
        "include/jemalloc/jemalloc_defs.h.in",
    ],
    outs = [
        "include/jemalloc/jemalloc_defs.h",
    ],
    cmd = "sed 's/#undef/#define/g' $(<) | " +
        "sed 's/#define LG_SIZEOF_PTR/#define LG_SIZEOF_PTR " + LG_SIZEOF_PTR + "/g' >$(@)",
)

PUBLIC_SYMBOLS = [
    "malloc_conf:malloc_conf",
    "malloc_message:malloc_message",
    "malloc:malloc",
    "calloc:calloc",
    "posix_memalign:posix_memalign",
    "aligned_alloc:aligned_alloc",
    "realloc:realloc",
    "free:free",
    "mallocx:mallocx",
    "rallocx:rallocx",
    "xallocx:xallocx",
    "sallocx:sallocx",
    "dallocx:dallocx",
    "sdallocx:sdallocx",
    "nallocx:nallocx",
    "mallctl:mallctl",
    "mallctlnametomib:mallctlnametomib",
    "mallctlbymib:mallctlbymib",
    "malloc_stats_print:malloc_stats_print",
    "malloc_usable_size:malloc_usable_size",
    "memalign:memalign",
    "valloc:valloc",
]
genrule(
    name = "jemalloc_rename_h",
    srcs = [
        "include/jemalloc/jemalloc_rename.sh",
    ],
    outs = [
        "include/jemalloc/jemalloc_rename.h",
    ],
    cmd = "tmpfile=$$(mktemp /tmp/jemalloc-public-symbols.XXXXXX); " +
        "printf " + "\\\\n".join(PUBLIC_SYMBOLS) + " >tmpfile;" +
        "$(<) tmpfile >$(@)"
)
genrule(
    name = "jemalloc_mangle_h",
    srcs = [
        "include/jemalloc/jemalloc_mangle.sh",
    ],
    outs = [
        "include/jemalloc/jemalloc_mangle.h",
    ],
    cmd = "tmpfile=$$(mktemp /tmp/jemalloc-public-symbols.XXXXXX); " +
        "printf " + "\\\\n".join(PUBLIC_SYMBOLS) + " >tmpfile;" +
        "$(<) tmpfile je_ >$(@)"
)

JEMALLOC_VERSION = "4.5.0-0-g04380e79f1e2428bd0ad000bbc6e3d2dfc6b66a5"
JEMALLOC_VERSION_MAJOR= "4"
JEMALLOC_VERSION_MINOR= "5"
JEMALLOC_VERSION_BUGFIX= "0"
JEMALLOC_VERSION_NREV= "0"
JEMALLOC_VERSION_GID = "04380e79f1e2428bd0ad000bbc6e3d2dfc6b66a5"
genrule(
    name = "jemalloc_macros_h",
    srcs = [
        "include/jemalloc/jemalloc_macros.h.in",
    ],
    outs = [
        "include/jemalloc/jemalloc_macros.h",
    ],
    cmd = "sed 's/@jemalloc_version@/" + JEMALLOC_VERSION + "/g' $(<) | " +
        "sed 's/@jemalloc_version_major@/" + JEMALLOC_VERSION_MAJOR + "/g' | " +
        "sed 's/@jemalloc_version_minor@/" + JEMALLOC_VERSION_MINOR + "/g' | " +
        "sed 's/@jemalloc_version_bugfix@/" + JEMALLOC_VERSION_BUGFIX + "/g' | " +
        "sed 's/@jemalloc_version_nrev@/" + JEMALLOC_VERSION_NREV + "/g' | " +
        "sed 's/@jemalloc_version_gid@/" + JEMALLOC_VERSION_GID + "/g' >$(@)",
)

genrule(
    name = "jemalloc_protos_h",
    srcs = [
        "include/jemalloc/jemalloc_protos.h.in",
    ],
    outs = [
        "include/jemalloc/jemalloc_protos.h",
    ],
    cmd = "sed 's/@//g' $(<) >$(@)"
)

genrule(
    name = "jemalloc_typedefs_h",
    srcs = [
        "include/jemalloc/jemalloc_typedefs.h.in",
    ],
    outs = [
        "include/jemalloc/jemalloc_typedefs.h",
    ],
    cmd = "cp $(<) $(@)"
)

genrule(
    name = "jemalloc_h",
    srcs = [
        ":jemalloc_defs_h",
        ":jemalloc_rename_h",
        ":jemalloc_macros_h",
        ":jemalloc_protos_h",
        ":jemalloc_typedefs_h",
        ":jemalloc_mangle_h",
        "include/jemalloc/jemalloc.sh",
    ],
    outs = [
        "include/jemalloc/jemalloc.h",
    ],
    cmd = "$(locations include/jemalloc/jemalloc.sh) $(GENDIR)/external/" + REPOSITORY_NAME[1:] + "/ >$(@)",
)

cc_library(
  name = "jemalloc",
  srcs = glob([
    "include/jemalloc/*.h",
    "include/jemalloc/internal/*.h",
  ])+ [
    ":internal_jemalloc_internal_h",
    ":internal_private_namespace_h",
    ":internal_size_classes_h",
    "src/jemalloc.c",
    "src/arena.c",
    "src/atomic.c",
    "src/base.c",
    "src/bitmap.c",
    "src/chunk.c",
    "src/chunk_dss.c",
    "src/chunk_mmap.c",
    "src/ckh.c",
    "src/ctl.c",
    "src/extent.c",
    "src/hash.c",
    "src/huge.c",
    "src/mb.c",
    "src/mutex.c",
    "src/nstime.c",
    "src/pages.c",
    "src/prng.c",
    "src/prof.c",
    "src/quarantine.c",
    "src/rtree.c",
    "src/spin.c",
    "src/stats.c",
    "src/tcache.c",
    "src/ticker.c",
    "src/tsd.c",
    "src/util.c",
    "src/witness.c",
  ],
  hdrs = [
    ":jemalloc_h",
  ],
  includes = [
    "include",
  ],
  copts = [
    "-D_GNU_SOURCE",
    "-D_REENTRANT",
    "-funroll-loops",
    "-fvisibility=hidden",
    "-std=gnu99",
    "-Werror=declaration-after-statement",
    "-Wsign-compare",
  ],
  deps = [
    "//external:jemalloc_config",
  ],
  visibility = ["//visibility:public"],
)
