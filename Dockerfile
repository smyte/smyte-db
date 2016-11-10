FROM ubuntu:xenial

ENV DEBIAN_FRONTEND noninteractive

# Bazel
RUN apt-get update
RUN apt-get install -y curl
RUN echo "deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN curl https://storage.googleapis.com/bazel-apt/doc/apt-key.pub.gpg | apt-key add -
RUN apt-get update && apt-get install -y bazel

# Smyte deps
RUN apt-get install -y libssl-dev git python

VOLUME /smyte
WORKDIR /smyte
CMD ["/bin/bash"]
