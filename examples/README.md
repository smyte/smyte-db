# examples

`smyte-db` can be used as a standalone, external [bazel](https://www.bazel.io) dependency to develop database services. In order to do that, first create a `WORKSPACE` file if you have not already and add the following content:
```
http_archive(
    name = "smyte",
    url = "https://github.com/smyte/smyte-db/archive/fb95ae0.tar.gz",
    strip_prefix = "smyte-db-fb95ae02e3bce04aeb4f9de2223777570492352e",
    sha256 = "7e1e60117a04f6051f5db91cffa2f60952f9b698091e3c8efa090957749e3ee5",
)

load("@smyte//third_party:workspace.bzl", "smyte_workspace")
smyte_workspace("smyte")
```
See [WORKSPACE](https://github.com/smyte/smyte-db/examples/WORKSPACE) for a complete example. Note that the SHA values
defined in the example are the latest SHAs of `smyte-db` as of writing. They need to be updated accordingly in order to
use newer versions.

Once `WORKSPACE` is created properly, libraries in `smyte-db` can be referenced directly in `BUILD` files as normal
[bazel](https://www.bazel.io) dependencies. [key_value](https://github.com/smyte/smyte-db/examples/key_value) is a
simple key-value store that uses redis protocol and persists data in [RockDB](http://rocksdb.org).
