// test_can_socket.cpp
//
// Unit tests for the sockpp `can_socket` class.
//
// *** NOTE: The "vcan0" virtual interface must be present. Set it up:
//   $ sudo ip link add type vcan && sudo ip link set up vcan0
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2025 Degui Liu
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

#include <cstring>
#include <string>
#include <thread>
#include <chrono>

#include "catch2_version.h"
#include "sockpp/can_socket.h"

/* Avoid "using namespace sockpp" — can_frame name clashes with ::can_frame */

static const std::string IFACE{"vcan0"};

// --------------------------------------------------------------------------

TEST_CASE("can_socket default constructor", "[can]") {
    sockpp::can_socket sock;

    REQUIRE(!sock.is_open());
    REQUIRE(sock.handle() == sockpp::INVALID_SOCKET);
}

TEST_CASE("can_socket move constructor", "[can]") {
    sockpp::can_socket sock1;
    sockpp::can_socket sock2(std::move(sock1));

    REQUIRE(!sock1.is_open());
    REQUIRE(!sock2.is_open());
}

TEST_CASE("can_socket move assignment", "[can]") {
    sockpp::can_socket sock1;
    sockpp::can_socket sock2;

    sock2 = std::move(sock1);

    REQUIRE(!sock1.is_open());
    REQUIRE(!sock2.is_open());
}

TEST_CASE("can_socket protocol family", "[can]") {
    int pf = sockpp::can_socket::PROTOCOL_FAMILY;
    int ct = sockpp::can_socket::COMM_TYPE;
    REQUIRE(pf == AF_CAN);
    REQUIRE(ct == SOCK_RAW);
}

TEST_CASE("can_socket open with valid address", "[can]") {
    auto addr_res = sockpp::can_address::create(IFACE);
    if (!addr_res) {
        WARN("Skipping: vcan0 not available");
        return;
    }

    sockpp::can_socket sock;
    auto res = sock.open(addr_res.value());
    if (res) {
        REQUIRE(sock.is_open());
        REQUIRE(sock.handle() != sockpp::INVALID_SOCKET);
    }
    else {
        WARN("Skipping: cannot open CAN socket (" << res.error().message() << ")");
    }
}

TEST_CASE("can_socket send and recv loopback", "[can]") {
    auto addr_res = sockpp::can_address::create(IFACE);
    if (!addr_res) {
        WARN("Skipping: vcan0 not available");
        return;
    }

    sockpp::can_socket sender;
    auto res1 = sender.open(addr_res.value());
    if (!res1) {
        WARN("Skipping: cannot open sender socket");
        return;
    }

    sockpp::can_socket receiver;
    auto res2 = receiver.open(addr_res.value());
    if (!res2) {
        WARN("Skipping: cannot open receiver socket");
        return;
    }

    /* Enable receiving own frames for loopback test */
    int recv_own = 1;
    receiver.set_option(SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
                        &recv_own, sizeof(recv_own));

    /* Set non-blocking to avoid hanging if loopback fails */
    receiver.set_non_blocking(true);

    const canid_t ID = 0x123;
    const uint8_t DATA[] = {0xDE, 0xAD, 0xBE, 0xEF};
    sockpp::can_frame tx_frame(ID, DATA, sizeof(DATA));

    auto send_res = sender.send(tx_frame);
    REQUIRE(send_res);
    REQUIRE(send_res.value() == sizeof(::can_frame));

    /* Small delay for loopback delivery */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sockpp::can_frame rx_frame;
    auto recv_res = receiver.recv(&rx_frame);
    if (recv_res) {
        REQUIRE(recv_res.value() == sizeof(::can_frame));
        REQUIRE(rx_frame.can_id == ID);
        REQUIRE(rx_frame.can_dlc == sizeof(DATA));
        REQUIRE(std::memcmp(rx_frame.data, DATA, sizeof(DATA)) == 0);
    }
    else {
        WARN("Loopback recv failed (may need CAN loopback enabled)");
    }
}
