// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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

#include "dccl/codec.h"
#include "dccl/codecs2/field_codec_default.h"

#include "dccl/binary.h"
#include "test.pb.h"

using namespace dccl::test;

int main(int argc, char* argv[])
{
    //    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    // check the empty messages
    dccl::Codec codec;

    codec.info<TestMsg>(&std::cout);
    codec.load<TestMsg>();

    {
        TestMsg msg_in, msg_out;

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

        codec.load(msg_in.GetDescriptor());

        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;

        codec.decode(bytes, &msg_out);

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

        assert(!msg_out.has_msg1());
        assert(!msg_out.msg1_repeat_size());
        assert(!msg_out.has_msg2());
        assert(!msg_out.msg2_repeat_size());
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    // check partially full messages
    {
        TestMsg msg_in, msg_out;

        msg_in.mutable_msg1()->set_val(0.1);
        msg_in.add_msg1_repeat()->set_val(0.11);
        msg_in.add_msg1_repeat()->set_val(0.12);
        msg_in.add_msg1_repeat()->set_val(0.13);
        msg_in.mutable_msg2()->set_val(0.2);
        msg_in.add_msg2_repeat()->set_val(0.21);
        msg_in.add_msg2_repeat()->set_val(0.22);
        msg_in.add_msg2_repeat()->set_val(0.23);

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;

        codec.decode(bytes, &msg_out);

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

        assert(msg_out.has_msg1());
        assert(msg_out.msg1_repeat_size() == 3);
        assert(msg_out.has_msg2());
        assert(msg_out.msg2_repeat_size() == 3);
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    // check message with set submessage but not children
    {
        TestMsg msg_in, msg_out;

        msg_in.mutable_msg1();
        msg_in.mutable_msg2()->set_val(1);
        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Try decode..." << std::endl;

        codec.decode(bytes, &msg_out);

        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

        assert(msg_out.has_msg1());
        assert(msg_out.has_msg2());
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }

    std::cout << "all tests passed" << std::endl;
}
