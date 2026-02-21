[中文](README_zh.md) | English

# sockpp

[![CI](https://github.com/DeguiLiu/sockpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/DeguiLiu/sockpp/actions/workflows/cmake-multi-platform.yml)
[![Code Coverage](https://github.com/DeguiLiu/sockpp/actions/workflows/coverage.yml/badge.svg)](https://github.com/DeguiLiu/sockpp/actions/workflows/coverage.yml)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Modern C++ socket library providing RAII-based socket wrappers for IPv4, IPv6, Unix Domain Sockets, CAN bus, and TLS. Forked from [fpagliughi/sockpp](https://github.com/fpagliughi/sockpp) with CI improvements and bug fixes.

## Fork Changes

- Cross-platform CI: Linux (gcc/clang) + Windows (MSVC)
- Code coverage workflow with lcov and Codecov integration
- Bug fix: `inet_address::resolve_name` and `inet6_address::resolve_name` use `WSAGetLastError()` instead of `errno` on Windows (upstream [#106](https://github.com/fpagliughi/sockpp/issues/106))

## Features

- IPv4 and IPv6 support (TCP/UDP)
- Unix Domain Sockets (Linux, macOS, BSD)
- CAN bus via SocketCAN (Linux)
- TLS support with OpenSSL or MbedTLS (experimental)
- C++17, RAII-based resource management
- Moveable socket objects (non-copyable)
- Cross-platform: Linux, macOS, Windows
- Suitable for ARM-Linux embedded systems

## Quick Start

```cmake
find_package(sockpp REQUIRED)
target_link_libraries(myapp Sockpp::sockpp)
```

## Building

### Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12 or later

### Build Commands

```bash
git clone https://github.com/DeguiLiu/sockpp.git
cd sockpp
cmake -B build
cmake --build build
cmake --install build
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| SOCKPP_BUILD_SHARED | ON | Build shared library |
| SOCKPP_BUILD_STATIC | OFF | Build static library |
| SOCKPP_BUILD_EXAMPLES | OFF | Build example programs |
| SOCKPP_BUILD_TESTS | OFF | Build unit tests (requires Catch2) |
| SOCKPP_WITH_UNIX_SOCKETS | ON (*nix) | Enable Unix Domain Sockets |
| SOCKPP_WITH_CAN | OFF | Enable CAN bus support (Linux) |
| SOCKPP_WITH_OPENSSL | OFF | Enable TLS with OpenSSL |
| SOCKPP_WITH_MBEDTLS | OFF | Enable TLS with MbedTLS |

## Usage Examples

### TCP Server

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

### TCP Client

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

### UDP Socket

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

## Thread Safety

Socket objects are not thread-safe. Use external synchronization (e.g. `std::mutex`) when sharing a socket across threads. Use `socket::clone()` to create a duplicate handle for one-reader-one-writer patterns.

## Documentation

Based on [fpagliughi/sockpp](https://github.com/fpagliughi/sockpp). See upstream [documentation](https://fpagliughi.github.io/sockpp/) for detailed API reference.

## License

BSD-3-Clause. See [LICENSE](LICENSE) for details.
