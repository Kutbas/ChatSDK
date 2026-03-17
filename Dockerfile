FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# 1. 安装基础编译环境与网络工具
RUN apt-get update && apt-get install -y \
    build-essential git sudo cmake pkg-config curl nano vim

# 2. 安装项目底层依赖 ("脚手架")
RUN apt-get install -y \
    libgflags-dev libfmt-dev libspdlog-dev libjsoncpp-dev \
    libgtest-dev libsqlite3-dev libssl-dev

# 3. 安装 cpp-httplib (Header-only)
RUN mkdir -p /workspace && cd /workspace && \
    git clone https://github.com/yhirose/cpp-httplib.git && \
    cp cpp-httplib/httplib.h /usr/include/

# 4. 获取项目源码并完成 SDK 编译与安装
RUN cd /workspace && \
    git clone https://github.com/Kutbas/ChatSDK.git && \
    cd ChatSDK/sdk && \
    mkdir build && cd build && \
    cmake .. && make && make install

# 5. 自动编译自带的 Demo 程序
RUN cd /workspace/ChatSDK/demo && \
    mkdir build && cd build && \
    cmake .. && make

# 6. 设置工作目录到 Demo 运行目录
WORKDIR /workspace/ChatSDK/demo/build

# 7. 默认启动 Demo 程序
CMD ["./AIChatDemo"]