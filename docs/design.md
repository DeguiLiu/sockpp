# sockpp 设计文档

## 概述

sockpp 是基于 Berkeley sockets 的现代 C++17 封装库，提供类型安全的 RAII 网络编程接口。
本 fork 针对 ARM-Linux 嵌入式场景优化，同时保持 Windows/macOS 跨平台兼容。

上游仓库: https://github.com/fpagliughi/sockpp
本仓库: https://github.com/DeguiLiu/sockpp

## 架构

### 层次结构

```
┌─────────────────────────────────────────────────────┐
│  协议层 (用户直接使用)                                │
│  tcp.h / udp.h / unix.h / can.h                    │
│  类型别名: tcp_socket, tcp_connector, udp_socket... │
├─────────────────────────────────────────────────────┤
│  Socket 层 (核心抽象)                                │
│  socket.h      -- RAII 基类, handle 生命周期管理      │
│  stream_socket -- 流式 I/O (read/write/read_n)      │
│  acceptor      -- 服务端监听 (bind/listen/accept)    │
│  connector     -- 客户端连接 (connect + 超时)         │
│  datagram_socket -- 无连接 UDP 风格                  │
│  raw_socket    -- 原始套接字                         │
├─────────────────────────────────────────────────────┤
│  地址层                                              │
│  sock_address.h    -- 地址基类                       │
│  inet_address.h    -- IPv4 地址 + DNS 解析           │
│  inet6_address.h   -- IPv6 地址 + DNS 解析           │
├─────────────────────────────────────────────────────┤
│  基础层                                              │
│  platform.h -- 平台适配 (Winsock/POSIX/SIGPIPE)     │
│  types.h    -- 类型别名 (duration/string/binary)     │
│  error.h    -- 错误码体系 (errc + gai_errc)          │
│  result.h   -- result<T> 错误传播 (类似 Rust Result) │
└─────────────────────────────────────────────────────┘
```

### 继承关系

```
socket (RAII base)
├── stream_socket  →  stream_socket_tmpl<ADDR>
│   └── connector  →  connector_tmpl<STREAM_SOCK>
├── acceptor       →  acceptor_tmpl<STREAM_SOCK>
├── datagram_socket → datagram_socket_tmpl<ADDR>
└── raw_socket     →  raw_socket_tmpl<ADDR>
```

## 文件结构

### 源文件 (src/)

| 文件 | 职责 |
|------|------|
| `socket.cpp` | error + socket + stream_socket + acceptor + connector + datagram_socket |
| `address.cpp` | inet_address (IPv4) + inet6_address (IPv6) |
| `unix/unix_address.cpp` | Unix domain 地址实现 |
| `linux/can_address.cpp` | CAN 地址实现 (Linux-only) |
| `linux/can_socket.cpp` | CAN 套接字实现 (Linux-only) |

### 头文件 (include/sockpp/)

| 分类 | 文件 | 说明 |
|------|------|------|
| 基础 | `platform.h` | 平台适配, Winsock/POSIX 类型定义 |
| 基础 | `types.h` | 时间/字符串类型别名 |
| 基础 | `error.h` | 错误码体系, gai_errc |
| 基础 | `result.h` | `result<T>` 模板, 错误传播 |
| Socket | `socket.h` | RAII 基类, 723 行 |
| Socket | `sock_address.h` | 地址基类 |
| Socket | `stream_socket.h` | 流式 socket 模板 |
| Socket | `datagram_socket.h` | 数据报 socket 模板 |
| Socket | `raw_socket.h` | 原始 socket 模板 |
| Socket | `acceptor.h` | 服务端监听模板 |
| Socket | `connector.h` | 客户端连接模板 |
| 地址 | `inet_address.h` | IPv4 地址 |
| 地址 | `inet6_address.h` | IPv6 地址 |
| 协议 | `tcp.h` | IPv4/IPv6 TCP 类型别名 |
| 协议 | `udp.h` | IPv4/IPv6 UDP 类型别名 |
| 协议 | `unix.h` | Unix domain 地址 + acceptor + 类型别名 |
| CAN | `can_frame.h` | CAN 帧 (Linux-only) |
| CAN | `can_address.h` | CAN 地址 (Linux-only) |
| CAN | `can_socket.h` | CAN 套接字 (Linux-only) |

## 设计原则

### RAII 资源管理

所有 socket 对象析构时自动关闭底层 handle，防止资源泄漏:

```cpp
{
    tcp_connector conn({"192.168.1.100", 8080});
    conn.write(data, len);
} // 自动 close
```

### Move-only 语义

Socket 可移动不可拷贝，支持所有权转移:

```cpp
auto sock = acceptor.accept();  // 返回值移动
worker_thread(std::move(sock)); // 转移到工作线程
```

### result\<T\> 错误传播

使用 `result<T>` 替代异常，兼容 `-fno-exceptions` 编译:

```cpp
auto res = connector.connect(addr);
if (!res) {
    // res.error() 返回 std::error_code
    log_error(res.error().message());
    return;
}
```

当编译器启用异常时，构造函数仍可抛出 `std::system_error`。
通过 `SOCKPP_THROW` 宏控制: 有异常时 throw，无异常时 abort。

### 编译期类型安全

通过模板参数化地址族，编译期绑定:

```cpp
// tcp_socket = stream_socket_tmpl<inet_address>
// tcp6_socket = stream_socket_tmpl<inet6_address>
// 编译期保证 IPv4 socket 不会误用 IPv6 地址
```

## 平台差异

| 特性 | Linux/POSIX | Windows |
|------|-------------|---------|
| 初始化 | `signal(SIGPIPE, SIG_IGN)` | `WSAStartup()` |
| 关闭 | `close()` | `closesocket()` |
| 非阻塞 | `fcntl(O_NONBLOCK)` | `ioctlsocket(FIONBIO)` |
| 错误码 | `errno` | `WSAGetLastError()` |
| 超时连接 | `poll()` | `select()` |
| Socket pair | `socketpair()` | 不支持 |
| Unix domain | 支持 | 有限支持 (Win10+) |
| SocketCAN | 支持 | 不支持 |
| 散布/聚集 I/O | `readv()/writev()` | `WSARecv()/WSASend()` |

## 与上游的差异

本 fork 相对于 fpagliughi/sockpp v2.0 的改动:

1. 修复 `resolve_name()` Windows 下使用 `errno` 而非 `WSAGetLastError()` 的 bug
2. 源文件合并: 8 个 .cpp → 2 个 (socket.cpp + address.cpp)
3. 头文件精简: 31 个 .h → 19 个 (合并 TCP/UDP/Unix typedef 文件)
4. CI 支持 Linux (gcc/clang) + Windows (MSVC) 矩阵
5. 中英文双语 README

## 构建

```bash
mkdir build && cd build
cmake .. -DSOCKPP_BUILD_SHARED=ON -DSOCKPP_BUILD_STATIC=ON \
         -DSOCKPP_BUILD_TESTS=ON -DSOCKPP_BUILD_EXAMPLES=ON
cmake --build . --config Release
ctest --output-on-failure
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `SOCKPP_BUILD_SHARED` | ON | 构建动态库 |
| `SOCKPP_BUILD_STATIC` | OFF | 构建静态库 |
| `SOCKPP_BUILD_TESTS` | OFF | 构建单元测试 |
| `SOCKPP_BUILD_EXAMPLES` | OFF | 构建示例程序 |
| `SOCKPP_WITH_CAN` | OFF | 启用 SocketCAN (Linux) |
| `SOCKPP_WITH_OPENSSL` | OFF | 启用 OpenSSL TLS |
| `SOCKPP_WITH_MBEDTLS` | OFF | 启用 MbedTLS |

## 嵌入式适配要点

- 支持 `-fno-exceptions -fno-rtti` 编译
- `result<T>` 零异常错误传播
- Move-only 语义，无隐式拷贝开销
- 模板参数化，编译期类型绑定，零虚函数开销
- 静态库链接，适合资源受限环境
