#include "pipeline/BuildVersion.h"

namespace pipeline {

// We expect external tools invoking bazel build would update these values.
const char* kSmyteBuildGitSha = "SMYTE_BUILD_GIT_SHA";
const char* kSmyteBuildGitTime = "SMYTE_BUILD_GIT_TIME";
const char* kSmyteBuildCompileTime = "SMYTE_BUILD_COMPILE_TIME";

}  // namespace pipeline
