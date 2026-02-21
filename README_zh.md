[English](README.md) | 中文

# sockpp

[![CI](https://github.com/DeguiLiu/sockpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/DeguiLiu/sockpp/actions/workflows/cmake-multi-platform.yml)
[![Code Coverage](https://github.com/DeguiLiu/sockpp/actions/workflows/coverage.yml/badge.svg)](https://github.com/DeguiLiu/sockpp/actions/workflows/coverage.yml)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

现代 C++ 网络套接字库，基于 RAII 设计，支持 IPv4/IPv6/Unix Domain/CAN bus/TLS(实验性)。

本仓库 fork 自 [fpagliughi/sockpp](https://github.com/fpagliughi/sockpp)，增加了 CI 改进和 Bug 修复。

## Fork 改进

- 跨平台 CI: Linux (gcc/clang) + Windows (MSVC)
- 代码覆盖率工作流集成
- Bug 修复: `inet_address::resolve_name` 和 `inet6_address::resolve_name` 在 Windows 上使用 `WSAGetLastError()` 替代 `errno` (上游 [#106](https://github.com/fpagliughi/sockpp/issues/106))

## 特性

- IPv4 & IPv6 (TCP/UDP)
- Unix Domain Sockets (*nix)
- CAN bus via SocketCAN (Linux)
- TLS 支持 OpenSSL 或 MbedTLS (实验性)
- C++17, RAII 资源管理
- 可移动套接字对象
- 跨平台: Linux, macOS, Windows
- 适用于 ARM-Linux 嵌入式系统

## 快速开始

```cmake
find_package(sockpp REQUIRED)
target_link_libraries(myapp Sockpp::sockpp)
```

## 构建

### 依赖

- C++17 编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12+

### 构建命令

```bash
git clone https://github.com/DeguiLiu/sockpp.git
cd sockpp
cmake -B build
cmake --build build
cmake --install build
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| SOCKPP_BUILD_SHARED | ON | 构建动态库 |
| SOCKPP_BUILD_STATIC | OFF | 构建静态库 |
| SOCKPP_BUILD_EXAMPLES | OFF | 构建示例程序 |
| SOCKPP_BUILD_TESTS | OFF | 构建单元测试 (需要 Catch2) |
| SOCKPP_WITH_UNIX_SOCKETS | ON (*nix) | 启用 Unix Domain Sockets |
| SOCKPP_WITH_CAN | OFF | 启用 CAN bus 支持 (Linux) |
| SOCKPP_WITH_OPENSSL | OFF | 启用 OpenSSL TLS |
| SOCKPP_WITH_MBEDTLS | OFF | 启用 MbedTLS TLS |

## 使用示例

### TCP 服务器

```cpp
#include "sockpp/tcp_acceptor.h"
#include <iostream>

int main() {
    sockpp::tcp_acceptor acc(12345);
    if (!acc) {
        std::cerr << "Error: " << acc.last_error_str() << std::endl;
        return 1;
    }

    while (true) {
        sockpp::tcp_socket sock = acc.accept();
        if (!sock) continue;

        char buf[512];
        ssize_t n = sock.read(buf, sizeof(buf));
        if (n > 0)
            sock.write(buf, n);  // Echo back
    }
}
```

### TCP 客户端

```cpp
#include "sockpp/tcp_connector.h"
#include <iostream>

int main() {
    sockpp::tcp_connector conn({"localhost", 12345});
    if (!conn) {
        std::cerr << "Error: " << conn.last_error_str() << std::endl;
        return 1;
    }

    conn.write("Hello, server!");

    char buf[512];
    ssize_t n = conn.read(buf, sizeof(buf));
    if (n > 0)
        std::cout.write(buf, n);
}
```

### UDP 通信

```cpp
#include "sockpp/udp_socket.h"

int main() {
    sockpp::udp_socket sock;
    sockpp::inet_address addr("localhost", 12345);

    std::string msg("Hello UDP!");
    sock.send_to(msg, addr);

    char buf[512];
    sockpp::inet_address src;
    ssize_t n = sock.recv_from(buf, sizeof(buf), &src);
}
```

## 线程安全

套接字对象本身不是线程安全的。多线程环境下需要外部同步 (如 `std::mutex`)。可使用 `socket::clone()` 创建句柄副本，实现一读一写的线程模式。

## 上游项目

基于 [fpagliughi/sockpp](https://github.com/fpagliughi/sockpp)。详细 API 参考请查阅上游[文档](https://fpagliughi.github.io/sockpp/)。

## 许可证

BSD-3-Clause. 详见 [LICENSE](LICENSE) 文件。
