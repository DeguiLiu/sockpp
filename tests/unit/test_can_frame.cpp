// test_can_frame.cpp
//
// Unit tests for the sockpp `can_frame` class.
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

#include "catch2_version.h"
#include "sockpp/can_frame.h"

/* Avoid "using namespace sockpp" — can_frame name clashes with ::can_frame */

// --------------------------------------------------------------------------

TEST_CASE("can_frame default constructor", "[can]") {
    sockpp::can_frame frame;

    REQUIRE(frame.can_id == 0);
    REQUIRE(frame.can_dlc == 0);

    /* All data bytes should be zero */
    for (int i = 0; i < CAN_MAX_DLEN; i++) {
        REQUIRE(frame.data[i] == 0);
    }
}

TEST_CASE("can_frame string data constructor", "[can]") {
    const canid_t ID = 0x123;
    const std::string DATA = "Hello";

    sockpp::can_frame frame(ID, DATA);

    REQUIRE(frame.can_id == ID);
    REQUIRE(frame.can_dlc == DATA.length());
    REQUIRE(std::memcmp(frame.data, DATA.data(), DATA.length()) == 0);
}

TEST_CASE("can_frame raw data constructor", "[can]") {
    const canid_t ID = 0x7FF;
    const uint8_t DATA[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const size_t LEN = sizeof(DATA);

    sockpp::can_frame frame(ID, DATA, LEN);

    REQUIRE(frame.can_id == ID);
    REQUIRE(frame.can_dlc == LEN);
    REQUIRE(std::memcmp(frame.data, DATA, LEN) == 0);
}

TEST_CASE("can_frame max data length", "[can]") {
    const canid_t ID = 0x100;
    const uint8_t DATA[CAN_MAX_DLEN] = {1, 2, 3, 4, 5, 6, 7, 8};

    sockpp::can_frame frame(ID, DATA, CAN_MAX_DLEN);

    REQUIRE(frame.can_id == ID);
    REQUIRE(frame.can_dlc == CAN_MAX_DLEN);
    REQUIRE(std::memcmp(frame.data, DATA, CAN_MAX_DLEN) == 0);
}

TEST_CASE("can_frame copy from C struct", "[can]") {
    ::can_frame raw{};
    raw.can_id = 0x456;
    raw.can_dlc = 3;
    raw.data[0] = 0xAA;
    raw.data[1] = 0xBB;
    raw.data[2] = 0xCC;

    sockpp::can_frame frame(raw);

    REQUIRE(frame.can_id == raw.can_id);
    REQUIRE(frame.can_dlc == raw.can_dlc);
    REQUIRE(frame.data[0] == 0xAA);
    REQUIRE(frame.data[1] == 0xBB);
    REQUIRE(frame.data[2] == 0xCC);
}

TEST_CASE("can_frame extended ID (EFF)", "[can]") {
    const canid_t ID = 0x12345678 | CAN_EFF_FLAG;
    const uint8_t DATA[] = {0x01};

    sockpp::can_frame frame(ID, DATA, 1);

    REQUIRE(frame.can_id == ID);
    REQUIRE((frame.can_id & CAN_EFF_FLAG) != 0);
    REQUIRE((frame.can_id & CAN_EFF_MASK) == 0x12345678);
    REQUIRE(frame.can_dlc == 1);
}

TEST_CASE("can_frame RTR flag", "[can]") {
    const canid_t ID = 0x200 | CAN_RTR_FLAG;
    const uint8_t dummy = 0;

    sockpp::can_frame frame(ID, &dummy, 0);

    REQUIRE(frame.can_id == ID);
    REQUIRE((frame.can_id & CAN_RTR_FLAG) != 0);
    REQUIRE(frame.can_dlc == 0);
}

TEST_CASE("can_frame zero-length data", "[can]") {
    const canid_t ID = 0x300;

    SECTION("from empty string") {
        sockpp::can_frame frame(ID, std::string{});
        REQUIRE(frame.can_id == ID);
        REQUIRE(frame.can_dlc == 0);
    }

    SECTION("from pointer with zero length") {
        const uint8_t dummy = 0;
        sockpp::can_frame frame(ID, &dummy, 0);
        REQUIRE(frame.can_id == ID);
        REQUIRE(frame.can_dlc == 0);
    }
}

TEST_CASE("can_frame sizeof matches C struct", "[can]") {
    REQUIRE(sizeof(sockpp::can_frame) == sizeof(::can_frame));
}
