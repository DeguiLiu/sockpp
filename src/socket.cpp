// socket.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2024 Frank Pagliughi
// Copyright (c) 2025 Degui Liu (DeguiLiu) - Consolidation and Windows fixes
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------

#include "sockpp/socket.h"

#include <fcntl.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "sockpp/acceptor.h"
#include "sockpp/connector.h"
#include "sockpp/datagram_socket.h"
#include "sockpp/error.h"
#include "sockpp/stream_socket.h"
#include "sockpp/version.h"

#if defined(SOCKPP_OPENSSL)
    #include <openssl/err.h>
    #include <openssl/ssl.h>
#endif

#if !defined(_WIN32)
    #include <sys/poll.h>
    #if defined(__APPLE__)
        #include <net/if.h>
    #endif
#endif

using namespace std;
using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
//                              error
/////////////////////////////////////////////////////////////////////////////

#if !defined(_WIN32)
const ::detail::gai_errc_category& gai_errc_category() {
    static ::detail::gai_errc_category c;
    return c;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//                           aux functions
/////////////////////////////////////////////////////////////////////////////

timeval to_timeval(const microseconds& dur) {
    const seconds sec = duration_cast<seconds>(dur);

    timeval tv;
#if defined(_WIN32)
    tv.tv_sec = long(sec.count());
#else
    tv.tv_sec = time_t(sec.count());
#endif
    tv.tv_usec = suseconds_t(duration_cast<microseconds>(dur - sec).count());
    return tv;
}

/////////////////////////////////////////////////////////////////////////////
//                         socket_initializer
/////////////////////////////////////////////////////////////////////////////

socket_initializer::socket_initializer() {
#if defined(_WIN32)
    WSADATA wsadata;
    ::WSAStartup(MAKEWORD(2, 0), &wsadata);
#else
    ::signal(SIGPIPE, SIG_IGN);
#endif

#if defined(SOCKPP_OPENSSL)
    SSL_library_init();
    SSL_load_error_strings();
#endif
}

socket_initializer::~socket_initializer() {
#if defined(_WIN32)
    ::WSACleanup();
#endif
}

void initialize() { socket_initializer::initialize(); }

/////////////////////////////////////////////////////////////////////////////
//                              socket
/////////////////////////////////////////////////////////////////////////////

result<> socket::close(socket_t h) noexcept {
#if defined(_WIN32)
    return check_res_none(::closesocket(h));
#else
    return check_res_none(::close(h));
#endif
}

result<socket> socket::create(int domain, int type, int protocol /*=0*/) noexcept {
    if (auto res = check_socket(::socket(domain, type, protocol)); !res)
        return res.error();
    else
        return socket(res.value());
}

result<socket> socket::clone() const {
    socket_t h = INVALID_SOCKET;
#if defined(_WIN32)
    WSAPROTOCOL_INFOW protInfo;
    if (::WSADuplicateSocketW(handle_, ::GetCurrentProcessId(), &protInfo) == 0)
        h = check_socket(
                ::WSASocketW(AF_INET, SOCK_STREAM, 0, &protInfo, 0, WSA_FLAG_OVERLAPPED)
        )
                .value();
#else
    h = ::dup(handle_);
#endif

    if (auto res = check_socket(h); !res)
        return res.error();

    return socket(h);
}

#if !defined(_WIN32)

result<int> socket::get_flags() const { return check_res(::fcntl(handle_, F_GETFL, 0)); }

result<> socket::set_flags(int flags) {
    return check_res_none(::fcntl(handle_, F_SETFL, flags));
}

result<> socket::set_flag(int flag, bool on /*=true*/) {
    auto res = get_flags();
    if (!res) {
        return res.error();
    }

    int flags = res.value();
    flags = on ? (flags | flag) : (flags & ~flag);
    return set_flags(flags);
}

bool socket::is_non_blocking() const {
    auto res = get_flags();
    return (res) ? ((res.value() & O_NONBLOCK) != 0) : false;
}

#endif

result<std::tuple<socket, socket>>
socket::pair(int domain, int type, int protocol /*=0*/) noexcept {
    result<std::tuple<socket, socket>> res;
    socket sock0, sock1;

#if !defined(_WIN32)
    int sv[2];

    if (::socketpair(domain, type, protocol, sv) == 0) {
        res = std::make_tuple<socket, socket>(socket{sv[0]}, socket{sv[1]});
    }
    else {
        res = result<std::tuple<socket, socket>>::from_last_error();
    }
#else
    (void)domain;
    (void)type;
    (void)protocol;

    res = errc::function_not_supported;
#endif

    return res;
}

void socket::reset(socket_t h /*=INVALID_SOCKET*/) noexcept {
    if (h != handle_) {
        std::swap(h, handle_);
        if (h != INVALID_SOCKET)
            close(h);
    }
}

result<> socket::bind(const sock_address& addr, int reuse) noexcept {
    if (reuse) {
#if defined(_WIN32) || defined(__CYGWIN__)
        if (reuse != SO_REUSEADDR) {
#else
        if (reuse != SO_REUSEADDR && reuse != SO_REUSEPORT) {
#endif
            return errc::invalid_argument;
        }

        if (auto res = set_option(SOL_SOCKET, reuse, true); !res) {
            return res;
        }
    }

    return check_res_none(::bind(handle_, addr.sockaddr_ptr(), addr.size()));
}

sock_address_any socket::address() const {
    auto addrStore = sockaddr_storage{};
    socklen_t len = sizeof(sockaddr_storage);

    auto res =
        check_res(::getsockname(handle_, reinterpret_cast<sockaddr*>(&addrStore), &len));
    if (!res)
        return sock_address_any{};

    return sock_address_any(addrStore, len);
}

sock_address_any socket::peer_address() const {
    auto addrStore = sockaddr_storage{};
    socklen_t len = sizeof(sockaddr_storage);

    auto res =
        check_res(::getpeername(handle_, reinterpret_cast<sockaddr*>(&addrStore), &len));
    if (!res)
        return sock_address_any{};

    return sock_address_any(addrStore, len);
}

result<> socket::get_option(
    int level, int optname, void* optval, socklen_t* optlen
) const noexcept {
    result<int> res;
#if defined(_WIN32)
    if (optval && optlen) {
        int len = static_cast<int>(*optlen);
        res = check_res(
            ::getsockopt(handle_, level, optname, static_cast<char*>(optval), &len)
        );
        if (res) {
            *optlen = static_cast<socklen_t>(len);
        }
    }
#else
    res = check_res(::getsockopt(handle_, level, optname, optval, optlen));
#endif
    return (res) ? error_code{} : res.error();
}

result<> socket::set_option(
    int level, int optname, const void* optval, socklen_t optlen
) noexcept {
#if defined(_WIN32)
    return check_res_none(
        ::setsockopt(
            handle_, level, optname, static_cast<const char*>(optval),
            static_cast<int>(optlen)
        )
    );
#else
    return check_res_none(::setsockopt(handle_, level, optname, optval, optlen));
#endif
}

result<> socket::set_non_blocking(bool on /*=true*/) {
#if defined(_WIN32)
    unsigned long mode = on ? 1 : 0;
    auto res = check_res(::ioctlsocket(handle_, FIONBIO, &mode));
    return (res) ? error_code{} : res.error();
#else
    return set_flag(O_NONBLOCK, on);
#endif
}

result<> socket::shutdown(int how /*=SHUT_RDWR*/) {
    if (handle_ == INVALID_SOCKET)
        return errc::invalid_argument;

    return check_res_none(::shutdown(handle_, how));
}

result<> socket::close() {
    if (handle_ != INVALID_SOCKET) {
        return close(release());
    }
    return error_code{};
}

result<size_t>
socket::recv_from(void* buf, size_t n, int flags, sock_address* srcAddr /*=nullptr*/) {
    sockaddr* p = srcAddr ? srcAddr->sockaddr_ptr() : nullptr;
    socklen_t len = srcAddr ? srcAddr->size() : 0;

#if defined(_WIN32)
    return check_res<ssize_t, size_t>(
        ::recvfrom(handle(), reinterpret_cast<char*>(buf), int(n), flags, p, &len)
    );
#else
    return check_res<ssize_t, size_t>(::recvfrom(handle(), buf, n, flags, p, &len));
#endif
}

/////////////////////////////////////////////////////////////////////////////
//                          stream_socket
/////////////////////////////////////////////////////////////////////////////

result<stream_socket> stream_socket::create(int domain, int protocol /*=0*/) {
    if (auto res = create_handle(domain, protocol); !res)
        return res.error();
    else
        return stream_socket{res.value()};
}

result<size_t> stream_socket::read(void* buf, size_t n) {
#if defined(_WIN32)
    auto cbuf = reinterpret_cast<char*>(buf);
    return check_res<ssize_t, size_t>(::recv(handle(), cbuf, int(n), 0));
#else
    return check_res<ssize_t, size_t>(::recv(handle(), buf, n, 0));
#endif
}

result<size_t> stream_socket::read_n(void* buf, size_t n) {
    uint8_t* b = reinterpret_cast<uint8_t*>(buf);
    size_t nx = 0;

    while (nx < n) {
        auto res = read(b + nx, n - nx);
        if (!res && res != errc::interrupted)
            return res.error();

        nx += size_t(res.value());
    }

    return nx;
}

result<size_t> stream_socket::read(const std::vector<iovec>& ranges) {
    if (ranges.empty())
        return 0;

#if !defined(_WIN32)
    return check_res<ssize_t, size_t>(::readv(handle(), ranges.data(), int(ranges.size())));
#else
    std::vector<WSABUF> bufs;
    for (const auto& iovec : ranges) {
        bufs.push_back(
            {static_cast<ULONG>(iovec.iov_len), static_cast<CHAR*>(iovec.iov_base)}
        );
    }

    DWORD flags = 0, nread = 0, nbuf = DWORD(bufs.size());

    auto ret = ::WSARecv(handle(), bufs.data(), nbuf, &nread, &flags, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
        return result<size_t>::from_last_error();
    return size_t(nread);
#endif
}

result<> stream_socket::read_timeout(const microseconds& to) {
    auto tv =
#if defined(_WIN32)
        DWORD(duration_cast<milliseconds>(to).count());
#else
        to_timeval(to);
#endif
    return set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

result<size_t> stream_socket::write(const void* buf, size_t n) {
#if defined(_WIN32)
    auto cbuf = reinterpret_cast<const char*>(buf);
    return check_res<ssize_t, size_t>(::send(handle(), cbuf, int(n), 0));
#else
    return check_res<ssize_t, size_t>(::send(handle(), buf, n, 0));
#endif
}

result<size_t> stream_socket::write_n(const void* buf, size_t n) {
    const uint8_t* b = reinterpret_cast<const uint8_t*>(buf);
    size_t nx = 0;

    while (nx < n) {
        auto res = write(b + nx, n - nx);
        if (!res && res != errc::interrupted)
            return res.error();
        nx += size_t(res.value());
    }

    return nx;
}

result<size_t> stream_socket::write(const std::vector<iovec>& ranges) {
#if !defined(_WIN32)
    return check_res<ssize_t, size_t>(::writev(handle(), ranges.data(), int(ranges.size())));
#else
    std::vector<WSABUF> bufs;
    for (const auto& iovec : ranges) {
        bufs.push_back(
            {static_cast<ULONG>(iovec.iov_len), static_cast<CHAR*>(iovec.iov_base)}
        );
    }

    DWORD nwritten = 0, nmsg = DWORD(bufs.size());

    if (::WSASend(handle(), bufs.data(), nmsg, &nwritten, 0, nullptr, nullptr) ==
        SOCKET_ERROR)
        return result<size_t>::from_last_error();
    return size_t(nwritten);
#endif
}

result<> stream_socket::write_timeout(const microseconds& to) {
    auto tv =
#if defined(_WIN32)
        DWORD(duration_cast<milliseconds>(to).count());
#else
        to_timeval(to);
#endif

    return set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/////////////////////////////////////////////////////////////////////////////
//                            acceptor
/////////////////////////////////////////////////////////////////////////////

result<acceptor> acceptor::create(int domain) noexcept {
    if (auto res = create_handle(domain); !res)
        return res.error();
    else
        return acceptor(res.value());
}

result<> acceptor::open(
    const sock_address& addr, int queSize /*=DFLT_QUE_SIZE*/, int reuse /*=0*/
) noexcept {
    if (is_open())
        return none{};

    if (auto res = create_handle(addr.family()); !res)
        return res.error();
    else
        reset(res.value());

    if (auto res = bind(addr, reuse); !res) {
        close();
        return res;
    }

    if (auto res = listen(queSize); !res) {
        close();
        return res;
    }

    return none{};
}

result<stream_socket> acceptor::accept(sock_address* clientAddr /*=nullptr*/) noexcept {
    sockaddr* p = clientAddr ? clientAddr->sockaddr_ptr() : nullptr;
    socklen_t len = clientAddr ? clientAddr->size() : 0;

    if (auto res = check_socket(::accept(handle(), p, clientAddr ? &len : nullptr)); !res)
        return res.error();
    else
        return stream_socket{res.value()};
}

/////////////////////////////////////////////////////////////////////////////
//                            connector
/////////////////////////////////////////////////////////////////////////////

result<> connector::recreate(const sock_address& addr) {
    if (auto res = create_handle(addr.family()); !res)
        return res.error();
    else {
        reset(res.value());
        return none{};
    }
}

result<none> connector::connect(const sock_address& addr) {
    if (auto res = recreate(addr); !res)
        return res;

    return check_res_none(::connect(handle(), addr.sockaddr_ptr(), addr.size()));
}

result<> connector::connect(const sock_address& addr, microseconds timeout) {
    if (timeout.count() <= 0)
        return connect(addr);

    if (auto res = recreate(addr); !res)
        return res;

    bool non_blocking =
#if defined(_WIN32)
        false;
#else
        is_non_blocking();
#endif

    if (!non_blocking)
        set_non_blocking(true);

    auto res = check_res(::connect(handle(), addr.sockaddr_ptr(), addr.size()));

    if (!res) {
        auto err = res.error();
        if (err == errc::operation_in_progress || err == errc::operation_would_block) {
#if defined(_WIN32)
            fd_set readset;
            FD_ZERO(&readset);
            FD_SET(handle(), &readset);
            fd_set writeset = readset;
            fd_set exceptset = readset;
            timeval tv = to_timeval(timeout);
            res =
                check_res(::select(int(handle()) + 1, &readset, &writeset, &exceptset, &tv));
#else
            pollfd fds = {handle(), POLLIN | POLLOUT, 0};
            int ms = int(duration_cast<milliseconds>(timeout).count());
            res = check_res(::poll(&fds, 1, ms));
#endif
            if (res) {
                if (res && res.value() > 0) {
                    int err;
                    if (get_option(SOL_SOCKET, SO_ERROR, &err))
                        res = result<int>::from_error(err);
                }
                else {
                    res = errc::timed_out;
                }
            }
        }

        if (!res) {
            close();
            return res.error();
        }
    }

    if (!non_blocking)
        set_non_blocking(false);

    return none{};
}

/////////////////////////////////////////////////////////////////////////////
//                         datagram_socket
/////////////////////////////////////////////////////////////////////////////

result<> datagram_socket::open(const sock_address& addr) noexcept {
    auto domain = addr.family();
    if (auto createRes = create_handle(domain); !createRes) {
        return createRes.error();
    }
    else {
        reset(createRes.value());
        if (auto res = bind(addr); !res) {
            close();
            return res;
        }
    }
    return none{};
}

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp
