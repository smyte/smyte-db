#ifndef PIPELINE_BUILDVERSION_H_
#define PIPELINE_BUILDVERSION_H_

namespace pipeline {

// BuildVersion.cpp contains placeholders for these variables.
// We expect external tools invoking bazel build would update these values.
extern const char* kSmyteBuildGitSha;
extern const char* kSmyteBuildGitTime;
extern const char* kSmyteBuildCompileTime;

}  // namespace pipeline

#endif  // PIPELINE_BUILDVERSION_H_
