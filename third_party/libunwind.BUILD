licenses(["notice"])

# Modified from https://github.com/mzhaom/trunk/blob/master/third_party/libunwind/BUILD

genrule(
    name = "libunwind_h",
    srcs = [
        "include/libunwind.h.in",
    ],
    outs = [
        "include/libunwind.h"
    ],
    # default to x86_64 arch
    cmd = "sed 's/@arch@/x86_64/' $(<) >$(@)",
)

genrule(
    name = "libunwind_common_h",
    srcs = [
        "include/libunwind-common.h.in",
    ],
    outs = [
        "libunwind-common.h"
    ],
    # define versions in compilation rule
    cmd = "sed '/#define UNW_VERSION_\(MAJOR\|MINOR\|EXTRA\)/d' $(<) >$(@)",
)

genrule(
    name = "tdep_libunwind_i_h",
    srcs = [
        "include/tdep/libunwind_i.h.in",
    ],
    outs = [
        "tdep/libunwind_i.h",
    ],
    # default to x86_64 arch
    cmd = "sed 's/@arch@/x86_64/' $(<) >$(@)",
)

PRIVATE_INCLUDE_SRCS = [
    "src/dwarf/Gstep.c",
    "src/dwarf/Gexpr.c",
    "src/dwarf/Gfde.c",
    "src/dwarf/Gfind_proc_info-lsb.c",
    "src/dwarf/Gfind_unwind_table.c",
    "src/dwarf/Gparser.c",
    "src/dwarf/Gpe.c",
    "src/mi/Gdestroy_addr_space.c",
    "src/mi/Gdyn-extract.c",
    "src/mi/Gfind_dynamic_proc_info.c",
    "src/mi/Gget_accessors.c",
    "src/mi/Gget_fpreg.c",
    "src/mi/Gget_proc_info_by_ip.c",
    "src/mi/Gget_proc_name.c",
    "src/mi/Gget_reg.c",
    "src/mi/Gput_dynamic_unwind_info.c",
    "src/mi/Gset_caching_policy.c",
    "src/mi/Gset_fpreg.c",
    "src/mi/Gset_reg.c",
    "src/x86_64/Ginit.c",
    "src/x86_64/Ginit_local.c",
    "src/x86_64/Ginit_remote.c",
    "src/x86_64/Gget_proc_info.c",
    "src/x86_64/Gget_save_loc.c",
    "src/x86_64/Gglobal.c",
    "src/x86_64/Gos-linux.c",
    "src/x86_64/Gregs.c",
    "src/x86_64/Gresume.c",
    "src/x86_64/Gstash_frame.c",
    "src/x86_64/Gstep.c",
    "src/x86_64/Gtrace.c",
]

cc_library(
    name = "libunwind",
    srcs = [
        "include/compiler.h",
        "include/dwarf.h",
        "include/dwarf-eh.h",
        "include/dwarf_i.h",
        "include/libunwind-dynamic.h",
        "include/libunwind_i.h",
        "include/mempool.h",
        "include/remote.h",
        "include/tdep-x86_64/dwarf-config.h",
        "include/tdep-x86_64/libunwind_i.h",
        "src/elfxx.h",
        "src/x86_64/unwind_i.h",
        "src/elf64.c",
        "src/os-linux.c",
        "src/dwarf/global.c",
        "src/dwarf/Lexpr.c",
        "src/dwarf/Lfde.c",
        "src/dwarf/Lfind_proc_info-lsb.c",
        "src/dwarf/Lfind_unwind_table.c",
        "src/dwarf/Lparser.c",
        "src/dwarf/Lpe.c",
        "src/dwarf/Lstep.c",
        "src/mi/backtrace.c",
        "src/mi/dyn-cancel.c",
        "src/mi/dyn-info-list.c",
        "src/mi/dyn-register.c",
        "src/mi/flush_cache.c",
        "src/mi/Gdyn-remote.c",
        "src/mi/init.c",
        "src/mi/Ldestroy_addr_space.c",
        "src/mi/Ldyn-extract.c",
        "src/mi/Lfind_dynamic_proc_info.c",
        "src/mi/Lget_fpreg.c",
        "src/mi/Lget_proc_info_by_ip.c",
        "src/mi/Lget_proc_name.c",
        "src/mi/Lget_reg.c",
        "src/mi/Lput_dynamic_unwind_info.c",
        "src/mi/Lset_caching_policy.c",
        "src/mi/Lset_fpreg.c",
        "src/mi/Lset_reg.c",
        "src/mi/mempool.c",
        "src/mi/strerror.c",
        "src/x86_64/getcontext.S",
        "src/x86_64/is_fpreg.c",
        "src/x86_64/Lcreate_addr_space.c",
        "src/x86_64/Lget_proc_info.c",
        "src/x86_64/Lget_save_loc.c",
        "src/x86_64/Lglobal.c",
        "src/x86_64/Linit.c",
        "src/x86_64/Linit_local.c",
        "src/x86_64/Linit_remote.c",
        "src/x86_64/Los-linux.c",
        "src/x86_64/Lregs.c",
        "src/x86_64/Lresume.c",
        "src/x86_64/Lstash_frame.c",
        "src/x86_64/Lstep.c",
        "src/x86_64/Ltrace.c",
        "src/x86_64/offsets.h",
        "src/x86_64/regname.c",
        "src/x86_64/setcontext.S",
    ] + PRIVATE_INCLUDE_SRCS,
    hdrs = [
        ":libunwind_h",
        ":libunwind_common_h",
        ":tdep_libunwind_i_h",
        "include/libunwind-x86_64.h",
        "src/elf64.h",
        "src/os-linux.h",
        "src/x86_64/init.h",
        "src/x86_64/ucontext_i.h",
        "src/x86_64/unwind_i.h",
        "src/x86_64/Gcreate_addr_space.c",
    ] + PRIVATE_INCLUDE_SRCS,
    textual_hdrs = [
        "src/elfxx.c",
    ],
    includes = [
        "include",
        "include/tdep-x86_64",
    ],
    copts = [
        "-DHAVE_CONFIG_H",
        "-D_GNU_SOURCE",
        "-fexceptions",
        "-Iexternal/" + REPOSITORY_NAME[1:] + "/src",
        "-Iexternal/" + REPOSITORY_NAME[1:] + "/src/x86_64",
        # version
        "-DUNW_VERSION_MAJOR=1",
        "-DUNW_VERSION_MINOR=1",
        "-DUNW_VERSION_EXTRA",
    ],
    deps = [
        "//external:libunwind_config",
    ],
    visibility = ["//visibility:public"],
)
