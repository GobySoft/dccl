// Copyright 2014-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
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

#include "../../native_protobuf/dccl_native_protobuf.h"
#include <google/protobuf/descriptor.pb.h>

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

dccl::Codec codec;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    codec.load<TestMsg>();
    codec.info<TestMsg>();
    {
        TestMsg msg_in;
        msg_in.add_a(0);
        msg_in.add_a(1);
        msg_in.add_a(2);

        msg_in.add_b(10);
        msg_in.add_b(11);

        msg_in.add_c(20);
        msg_in.add_c(21);
        msg_in.add_c(22);

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;
        std::cout << codec.max_size(msg_in.GetDescriptor()) << std::endl;

        TestMsg msg_out;
        codec.decode(bytes, &msg_out);

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    {
        TestMsg msg_in;
        msg_in.add_a(0);
        msg_in.add_a(1);
        msg_in.add_a(2);
        msg_in.add_a(3);
        msg_in.add_a(4);
        msg_in.add_a(5);

        msg_in.add_b(10);
        msg_in.add_b(11);

        msg_in.add_c(20);
        msg_in.add_c(21);
        msg_in.add_c(22);

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;
        std::cout << codec.max_size(msg_in.GetDescriptor()) << std::endl;

        TestMsg msg_out;
        codec.decode(bytes, &msg_out);

        msg_in.mutable_a()->RemoveLast();

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    {
        TestMsg msg_in;
        msg_in.add_a(0);
        msg_in.add_a(1);
        msg_in.add_a(2);
        msg_in.add_a(3);
        msg_in.add_a(4);

        msg_in.add_c(20);
        msg_in.add_c(21);
        msg_in.add_c(22);

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;
        std::cout << codec.max_size(msg_in.GetDescriptor()) << std::endl;

        TestMsg msg_out;
        codec.decode(bytes, &msg_out);

        auto bmin = dccl::test::TestMsg::descriptor()
                        ->FindFieldByName("b")
                        ->options()
                        .GetExtension(dccl::field)
                        .min();
        msg_in.add_b(bmin);
        msg_in.add_b(bmin);

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    try
    {
        // should throw exception on missing max repeat
        codec.load<InvalidTestMsgMissingMaxRepeat>();
        assert(false);
    }
    catch (const dccl::Exception& e)
    {
        // expected
    }

    try
    {
        // should throw exception
        codec.load<InvalidTestMsgMaxRepeatLessThanOne>();
        assert(false);
    }
    catch (const dccl::Exception& e)
    {
        // expected
    }

    try
    {
        // should throw exception
        codec.load<InvalidTestMsgMaxRepeatLessThanMinRepeat>();
        assert(false);
    }
    catch (const dccl::Exception& e)
    {
        // expected
    }

    std::cout << "All tests passed." << std::endl;
}
