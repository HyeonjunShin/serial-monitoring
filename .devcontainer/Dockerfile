FROM ubuntu:22.04

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive && \
    apt install -y --no-install-recommends \
    # Basic tools
    cmake \
    gdb \
    sudo \
    git \
    build-essential \
    # clangd
    wget curl lsb-release software-properties-common gnupg \
    # boost
    libboost-dev \
    # Qt6
    qt6-base-dev qt6-tools-dev libgl1-mesa-dev \
    && apt clean && rm -rf /var/lib/apt/lists/*

# setting clangd 21 version(https://apt.llvm.org/)
RUN wget https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh \
    && chmod +x /tmp/llvm.sh \ 
    && /tmp/llvm.sh 21 \
    && ln -s /usr/bin/clangd-21 /usr/bin/clangd 
    
# etc...
SHELL ["/bin/bash", "-c"]

RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash \
    && source ~/.nvm/nvm.sh \
    && nvm install 24 \
    && npm install -g @google/gemini-cli@latest

# Using non-root system
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USERNAME -m -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
    
USER $USERNAME

CMD ["/bin/bash"]

