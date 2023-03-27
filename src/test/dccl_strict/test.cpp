// Copyright 2019-2022:
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
// tests all protobuf types with _default codecs, repeat and non repeat

#include <fstream>

#include <google/protobuf/descriptor.pb.h>

#include "dccl/binary.h"
#include "dccl/codec.h"
#include "test.pb.h"
using namespace dccl::test;

void decode_check(const TestMsg& msg_in);
dccl::Codec codec;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    codec.info<TestMsg>();
    codec.load<TestMsg>();

    // enable strict mode
    codec.set_strict(true);

    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));
        msg_in.add_ri(1);
        msg_in.add_ri(2);
        msg_in.add_ri(3);

        decode_check(msg_in);
    }

    // double out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(150);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("d"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // int out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(-30);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("i"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // string (version 2) out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s2("foobar1234546789");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("s2"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // string (version 3) out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foobar1234546789");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("s"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // bytes out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(12, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("b"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // var bytes out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_vb(std::string(12, '1'));

        decode_check(msg_in);
        bool expected_out_of_range_exception = false;
        assert(expected_out_of_range_exception);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("vb"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // repeat size out of range
    try
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));
        msg_in.add_ri(1);
        msg_in.add_ri(2);
        msg_in.add_ri(3);
        msg_in.add_ri(4);

        decode_check(msg_in);
    }
    catch (dccl::OutOfRangeException& e)
    {
        assert(e.field() == TestMsg::descriptor()->FindFieldByName("ri"));
        std::cout << "Caught (as expected) " << e.what() << std::endl;
    }

    // disable strict mode
    codec.set_strict(false);

    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));
        msg_in.add_ri(1);
        msg_in.add_ri(2);
        msg_in.add_ri(3);

        decode_check(msg_in);
    }

    // double out of range
    {
        TestMsg msg_in;
        msg_in.set_d(150);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
    }

    // int out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(-30);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
    }

    // string (version 2) out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s2("foobar1234546789");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
    }

    // string (version 3) out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foobar1234546789");
        msg_in.set_b(std::string(9, '1'));

        decode_check(msg_in);
    }

    // bytes out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(12, '1'));

        decode_check(msg_in);
    }

    // var bytes out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_vb(std::string(12, '1'));

        decode_check(msg_in);
    }

    // repeat size out of range
    {
        TestMsg msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);
        msg_in.set_s("foo");
        msg_in.set_b(std::string(9, '1'));
        msg_in.add_ri(1);
        msg_in.add_ri(2);
        msg_in.add_ri(3);
        msg_in.add_ri(4);

        decode_check(msg_in);
    }

    std::cout << "all tests passed" << std::endl;
}

void decode_check(const TestMsg& msg_in)
{
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

    std::cout << "Try encode (in bounds)..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;

    TestMsg msg_out;
    codec.decode(bytes, &msg_out);

    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
}
