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
#include "test2.pb.h"
#include "test3.pb.h"

using namespace dccl::test;

// Test 1: encode/decode with DCCL-required and DCCL-optional fields populated
void test1(dccl::Codec& codec, dccl::Codec& codec2)
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
    assert(!msg_out.has_child());    
    
    Proto2Msg msg2_out;
    codec2.decode(encoded, &msg2_out);
    
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg in: " << msg_in.ShortDebugString() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg out: " << msg_out.ShortDebugString() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg2 out: " << msg2_out.ShortDebugString() << std::endl;

    assert(msg_in.req_i32() == msg2_out.req_i32());
    assert(msg_in.req_ui32() == msg2_out.req_ui32());
    assert(msg2_out.has_opt_i32());
    assert(msg_in.opt_i32() == msg2_out.opt_i32());
    assert(msg2_out.has_opt_ui32());
    assert(msg_in.opt_ui32() == msg2_out.opt_ui32());
    assert(!msg2_out.has_child());
}

// Test 2: encode/decode with optional fields left empty
void test2(dccl::Codec& codec)
{
    Proto3Msg msg_in;
    msg_in.set_req_i32(0);
    msg_in.set_req_ui32(10);
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

// Test 3: encode/decode with required fields left empty
void test3(dccl::Codec& codec)
{
    Proto3Msg msg_in;

    std::string encoded;
    codec.encode(&encoded, msg_in);

    Proto3Msg msg_out;
    codec.decode(encoded, &msg_out);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg in: " << msg_in.ShortDebugString() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg out: " << msg_out.ShortDebugString() << std::endl;

    assert(msg_in.req_i32() == msg_out.req_i32());

    // built-in default of 0 is not encodable with this range of DCCL values
    assert(msg_out.req_ui32() == 10);
}

// Test 4: encode child message
void test4(dccl::Codec& codec)
{
    Proto3Msg msg_in;
    msg_in.mutable_child()->set_req_dbl(0.4);

    std::string encoded;
    codec.encode(&encoded, msg_in);

    Proto3Msg msg_out;
    codec.decode(encoded, &msg_out);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg in: " << msg_in.ShortDebugString() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Msg out: " << msg_out.ShortDebugString() << std::endl;

    assert(msg_in.req_i32() == msg_out.req_i32());
}

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    dccl::Codec codec3;
    dccl::Codec codec2;
    codec3.load<Proto3Msg>();
    codec2.load<Proto2Msg>();
    codec3.info<Proto3Msg>(&dccl::dlog);
    codec2.info<Proto2Msg>(&dccl::dlog);

    test1(codec3, codec2);
    test2(codec3);
    test3(codec3);
    test4(codec3);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
    return 0;
}
