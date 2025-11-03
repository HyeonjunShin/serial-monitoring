FROM ubuntu:22.04

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive && \
    apt install -y --no-install-recommends \
    git \
    curl \
    unzip \
    ca-certificates \
    build-essential \
    openjdk-17-jdk \
    wget lsb-release software-properties-common gnupg \
    && apt clean && rm -rf /var/lib/apt/lists/*

ARG BAZELISK_VERSION="v1.27.0"
RUN curl -LO "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-amd64" && \
    chmod +x bazelisk-linux-amd64 && \
    mv bazelisk-linux-amd64 /usr/local/bin/bazel

ARG BUILDIFIER_VERSION="v8.2.1"
RUN curl -LO "https://github.com/bazelbuild/buildtools/releases/download/${BUILDIFIER_VERSION}/buildifier-linux-amd64" && \
    chmod +x buildifier-linux-amd64 && \
    mv buildifier-linux-amd64 /usr/local/bin/buildifier

# setting clangd 21 version(https://apt.llvm.org/)
RUN apt update && apt install -y wget lsb-release software-properties-common gnupg \
    && wget https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh \
    && chmod +x /tmp/llvm.sh \ 
    && /tmp/llvm.sh 21 \
    && ln -s /usr/bin/clangd-21 /usr/bin/clangd 

# RUN apt update && \
#     apt install -y apt-transport-https curl gnupg wget lsb-release software-properties-common git && \
#     curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > /usr/share/keyrings/bazel-archive-keyring.gpg && \
#     echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list > /dev/null && \
#     apt update && \
#     apt install -y bazel

# RUN curl https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh && \
#     chmod +x /tmp/llvm.sh && \ 
#     /tmp/llvm.sh 19 && \
#     rm -f /tmp/llvm.sh && \
#     ln -s /usr/bin/clangd-19 /usr/bin/clangd


CMD ["/bin/bash"]


# ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# # Optionally install the cmake for vcpkg
# COPY ./reinstall-cmake.sh /tmp/

# RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
#         chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
#     fi \
#     && rm -f /tmp/reinstall-cmake.sh

# # [Optional] Uncomment this section to install additional vcpkg ports.
# # RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# # [Optional] Uncomment this section to install additional packages.
# # RUN apt update && export DEBIAN_FRONTEND=noninteractive \
# #     && apt -y install --no-install-recommends <your-package-list-here>



