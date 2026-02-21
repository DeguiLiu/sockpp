/**
 * @file tcp.h
 *
 * IPv4/IPv6 TCP socket type aliases: socket, connector, acceptor.
 *
 * @author Frank Pagliughi
 * @author Degui Liu (DeguiLiu)
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019-2024 Frank Pagliughi
// Copyright (c) 2025 Degui Liu (DeguiLiu)
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

#ifndef __sockpp_tcp_h
#define __sockpp_tcp_h

#include "sockpp/acceptor.h"
#include "sockpp/connector.h"
#include "sockpp/inet_address.h"
#include "sockpp/inet6_address.h"
#include "sockpp/stream_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
// IPv4 TCP

/** IPv4 streaming TCP socket */
using tcp_socket = stream_socket_tmpl<inet_address>;

/** IPv4 TCP connector (client) */
using tcp_connector = connector_tmpl<tcp_socket>;

/** IPv4 TCP acceptor (server) */
using tcp_acceptor = acceptor_tmpl<tcp_socket>;

/////////////////////////////////////////////////////////////////////////////
// IPv6 TCP

/** IPv6 streaming TCP socket */
using tcp6_socket = stream_socket_tmpl<inet6_address>;

/** IPv6 TCP connector (client) */
using tcp6_connector = connector_tmpl<tcp6_socket>;

/** IPv6 TCP acceptor (server) */
using tcp6_acceptor = acceptor_tmpl<tcp6_socket>;

/////////////////////////////////////////////////////////////////////////////
}  // namespace sockpp

#endif  // __sockpp_tcp_h
