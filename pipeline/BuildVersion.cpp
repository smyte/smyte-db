#include "pipeline/BuildVersion.h"

namespace pipeline {

// We expect external tools invoking bazel build would update these values.
const char* kSmyteBuildGitSha = "d5f56b2c66f3efa3a4fc33b14c2b46be7af74f09";
const char* kSmyteBuildGitTime = "2017-03-17T20:24:09+00:00";
const char* kSmyteBuildCompileTime = "2017-03-17T20:38:24+00:00";

}  // namespace pipeline
