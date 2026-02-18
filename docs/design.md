# sockpp Design Documentation

## Overview

sockpp is a modern C++17 wrapper around Berkeley sockets, providing a clean, type-safe interface for network programming on POSIX and Windows systems.

## Architecture

### Core Components

- **socket** - Base class wrapping system socket handles with RAII lifetime management
- **acceptor** - Server-side socket for accepting incoming connections
- **connector** - Client-side socket for establishing outgoing connections
- **stream_socket** - Bidirectional communication socket
- **datagram_socket** - Connectionless UDP-style socket

### Platform Support

- **Linux** - Full support including SocketCAN
- **macOS** - Full support
- **Windows** - Full support with Winsock2
- **POSIX systems** - Unix-domain sockets on *nix platforms

### Protocol Support

- **IPv4** - TCP and UDP
- **IPv6** - TCP and UDP
- **Unix-domain sockets** - Stream and datagram (POSIX systems)
- **SocketCAN** - CAN bus programming (Linux)
- **TLS/SSL** - Experimental support via OpenSSL or MbedTLS

## Design Principles

### 1. RAII Resource Management

All socket objects use RAII pattern. When a socket object goes out of scope, the underlying system socket is automatically closed. This prevents resource leaks and ensures proper cleanup.

```cpp
{
    sockpp::tcp_socket sock;
    sock.connect({"localhost", 8080});
    // ... use socket
} // Socket automatically closed here
```

### 2. Move Semantics

Sockets are moveable but not copyable. This allows transferring socket ownership between scopes and threads:

```cpp
sockpp::tcp_socket sock1 = create_socket();
sockpp::tcp_socket sock2 = std::move(sock1);  // Transfer ownership
// sock1 is now invalid
```

### 3. Error Handling with result<T>

Modern error handling using `result<T>` type instead of exceptions:

```cpp
auto result = sock.connect({"localhost", 8080});
if (!result) {
    std::cerr << "Connection failed: " << result.error().message() << std::endl;
}
```

### 4. Zero-Cost Abstractions

The library provides high-level abstractions without runtime overhead. Template specialization and inline functions ensure optimal performance.

## Build System

- **CMake 3.12+** - Modern build configuration
- **Header-only option** - Can be used as header-only library
- **Modular builds** - Optional TLS support (OpenSSL or MbedTLS)
- **Cross-platform** - Single CMakeLists.txt for all platforms

## Testing

- **Unit tests** - Comprehensive test coverage using standard C++ testing frameworks
- **Platform testing** - CI/CD on Linux, macOS, and Windows
- **TLS testing** - Separate test suites for OpenSSL and MbedTLS backends

## Future Development

- Complete TLS/SSL support (currently experimental)
- Enhanced documentation and examples
- Performance optimizations
- Additional protocol support as needed

## References

- Official documentation: https://fpagliughi.github.io/sockpp/
- GitHub repository: https://github.com/fpagliughi/sockpp
- C++17 standard: https://en.cppreference.com/w/cpp/17
