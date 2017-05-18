# smyte-db

This repo is the central build environment for all Smyte C++ projects. For documentation, issues, and code for the submodules in this project, please see each project's individual repo:
  * [counters](https://github.com/smyte/counters)
  * [ratelimit](https://github.com/smyte/ratelimit)

Smyte's C++ development is based on facebook's [folly](https://github.com/facebook/folly) and [wangle](https://github.com/facebook/wangle) libraries to build high performance servers. The build system uses Google's [bazel](http://bazel.io/). By default, it builds a single executable binary that can be deployed to production directly.

## Setup
* Install `bazel`, `libssl-dev`, and `libatomic1`.
* Ensure submodules are up-to-date: `git submodule update --init`.
* Build a project: `bazel clean && bazel build ratelimit`. The first build may take a while to fetch all the third-party dependencies and build static libraries.

## Style guide
Follow [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html) with a few exceptions:
* Using C++ exception is allowed because both `folly` and `wangle` use it extensively.
* Each line can have up to 120 characters instead of 80. Google sticks with 80 for historical reasons, but we have much wider screens now.
* Method names use camel case, which starts with a lower case letter, e.g., `foo.addBack()`, instead of pascal case, which starts with a capital letter, e.g., `foo.AddBack()`. The main reason is that both `folly` and `wangle` use camel case, and we want to be consistent. Note that `rocksdb` uses pascal case because it is derived from Google's `leveldb` code. While we also use `rocksdb` heavily, we only use it as a library as supposed to involving it in the inheritance hierarchy.
* Use [cpplint](https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py) to check for style violations. We use the following config:
```
  --filter=-legal/copyright,-build/c++11,
  --linelength=120,
```
* Name files using camel case and local variables using pascal case for the same reason.
* Use `.cpp` instead of `.cc` as extension for C++ souce code files. There is really no difference between the two. Google uses `.cc`, and its build rule is also called `cc_binary` for example. But we'd rather be consistent with the main facebook C++ libraries.
* Use [Clang Format](http://clang.llvm.org/docs/ClangFormat.html) to format your source code with the following config:
```yaml
  BasedOnStyle: Google
  ColumnLimit: 119
  DerivePointerAlignment: false
  PointerAlignment: Left
  AccessModifierOffset: -1
```

## Test
To run all tests, simply execute `bazel test //...:all`.
