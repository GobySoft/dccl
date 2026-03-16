// Copyright 2023:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

// Tests proto3 syntax support via (dccl.field).required = true

#include <cassert>
#include <iostream>

#include "../../codec.h"
#include "test.pb.h"

using namespace dccl::test;

// Test 1: encode/decode with DCCL-required and DCCL-optional fields populated
void test1(dccl::Codec& codec)
{
    Proto3Msg msg_in;
    msg_in.set_req_i32(-50);
    msg_in.set_req_ui32(512);
    msg_in.set_opt_i32(200);
    msg_in.set_opt_ui32(100);

    std::string encoded;
    codec.encode(&encoded, msg_in);

    Proto3Msg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.req_i32() == msg_out.req_i32());
    assert(msg_in.req_ui32() == msg_out.req_ui32());
    assert(msg_out.has_opt_i32());
    assert(msg_in.opt_i32() == msg_out.opt_i32());
    assert(msg_out.has_opt_ui32());
    assert(msg_in.opt_ui32() == msg_out.opt_ui32());
}

// Test 2: encode/decode with optional fields left empty
void test2(dccl::Codec& codec)
{
    Proto3Msg msg_in;
    msg_in.set_req_i32(0);
    msg_in.set_req_ui32(0);
    // leave optional fields unset

    std::string encoded;
    codec.encode(&encoded, msg_in);

    Proto3Msg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.req_i32() == msg_out.req_i32());
    assert(msg_in.req_ui32() == msg_out.req_ui32());
    assert(!msg_out.has_opt_i32());
    assert(!msg_out.has_opt_ui32());
}

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;
    codec.load<Proto3Msg>();
    codec.info<Proto3Msg>(&dccl::dlog);

    test1(codec);
    test2(codec);

    std::cout << "all tests passed" << std::endl;
    return 0;
}
