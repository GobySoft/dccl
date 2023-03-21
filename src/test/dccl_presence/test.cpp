// Copyright 2019:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Kyle Guilbert <kguilbert@aphysci.com>
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
// tests required versus optional encoding of fields using a presence bit

#include "dccl/codec.h"
#include "test/dccl_presence/test.pb.h"
using namespace dccl::test;

using dccl::operator<<;

void test1(dccl::Codec&);
void test2(dccl::Codec&);

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    dccl::Codec codec;
    codec.load<PresenceMsg>();
    codec.info<PresenceMsg>(&dccl::dlog);
    test1(codec);
    test2(codec);

    std::cout << "all tests passed" << std::endl;
}

// optional fields all left empty
void test1(dccl::Codec& codec)
{
    PresenceMsg msg_in;

    msg_in.set_req_i32(-100);
    msg_in.set_req_i64(65535);
    msg_in.set_req_ui32(1022);
    msg_in.set_req_ui64(101);
    msg_in.set_req_float(900.12345);
    msg_in.set_req_double(900.12345678);
    msg_in.set_req_enum(ENUM2_C);

    std::string encoded;
    codec.encode(&encoded, msg_in);

    assert(encoded.size() == 17);
    
    PresenceMsg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.req_i32() == msg_out.req_i32());
    assert(msg_in.req_i64() == msg_out.req_i64());
    assert(msg_in.req_ui32() == msg_out.req_ui32());
    assert(msg_in.req_ui64() == msg_out.req_ui64());
    assert(fabsf(msg_in.req_float() - msg_out.req_float()) < 1e-4);
    assert(fabs(msg_in.req_double() - msg_out.req_double()) < 1e-7);
    assert(msg_in.req_enum() == msg_out.req_enum());

    assert(!msg_out.has_opt_i32());
    assert(!msg_out.has_opt_i64());
    assert(!msg_out.has_opt_ui32());
    assert(!msg_out.has_opt_ui64());
    assert(!msg_out.has_opt_float());
    assert(!msg_out.has_opt_double());
    assert(!msg_out.has_opt_enum());

    assert(msg_out.repeat_i32_size() == 0);
    assert(msg_out.repeat_enum_size() == 0);

    std::cout << "test1 passed" << std::endl;
}

// all fields populated
void test2(dccl::Codec& codec)
{
    PresenceMsg msg_in;

    msg_in.set_req_i32(500);
    msg_in.set_req_i64(0);
    msg_in.set_req_ui32(0);
    msg_in.set_req_ui64(100);
    msg_in.set_req_float(-900.12345);
    msg_in.set_req_double(-900.12345678);
    msg_in.set_req_enum(ENUM2_A);

    msg_in.set_opt_i32(-100);
    msg_in.set_opt_i64(65535);
    msg_in.set_opt_ui32(0);
    msg_in.set_opt_ui64(1123);
    msg_in.set_opt_float(-900.12345);
    msg_in.set_opt_double(900.12345678);
    msg_in.set_opt_enum(ENUM2_A);

    msg_in.add_repeat_i32(500);
    msg_in.add_repeat_enum(ENUM2_A);
    msg_in.add_repeat_enum(ENUM2_B);
    msg_in.add_repeat_enum(ENUM2_C);


    std::string encoded;
    codec.encode(&encoded, msg_in);

    assert(encoded.size() == 33);

    PresenceMsg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.req_i32() == msg_out.req_i32());
    assert(msg_in.req_i64() == msg_out.req_i64());
    assert(msg_in.req_ui32() == msg_out.req_ui32());
    assert(msg_in.req_ui64() == msg_out.req_ui64());
    assert(fabsf(msg_in.req_float() - msg_out.req_float()) < 1e-4);
    assert(fabs(msg_in.req_double() - msg_out.req_double()) < 1e-7);
    assert(msg_in.req_enum() == msg_out.req_enum());

    assert(msg_out.has_opt_i32());
    assert(msg_out.has_opt_i64());
    assert(msg_out.has_opt_ui32());
    assert(msg_out.has_opt_ui64());
    assert(msg_out.has_opt_float());
    assert(msg_out.has_opt_double());
    assert(msg_out.has_opt_enum());

    assert(msg_in.opt_i32() == msg_out.opt_i32());
    assert(msg_in.opt_i64() == msg_out.opt_i64());
    assert(msg_in.opt_ui32() == msg_out.opt_ui32());
    assert(msg_in.opt_ui64() == msg_out.opt_ui64());
    assert(fabsf(msg_in.opt_float() - msg_out.opt_float()) < 1e-4);
    assert(fabs(msg_in.opt_double() - msg_out.opt_double()) < 1e-7);
    assert(msg_in.opt_enum() == msg_out.opt_enum());

    assert(msg_in.repeat_i32_size() == msg_out.repeat_i32_size());
    assert(std::equal(msg_in.repeat_i32().begin(), msg_in.repeat_i32().end(), msg_out.repeat_i32().begin()));
    assert(msg_in.repeat_enum_size() == msg_out.repeat_enum_size());
    assert(std::equal(msg_in.repeat_enum().begin(), msg_in.repeat_enum().end(), msg_out.repeat_enum().begin()));


    std::cout << "test2 passed" << std::endl;
}
