// Copyright 2011-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Davide Fenucci <davfen@noc.ac.uk>
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

// ensure we link in dccl_native_protobuf.so
dccl::native_protobuf::EnumFieldCodec dummy;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    codec.load_library(DCCL_NATIVE_PROTOBUF_NAME);

    {
        TestMsg msg_in;
        msg_in.set_double_oneof1(10.56);
        msg_in.mutable_msg_oneof2()->set_val(100.123);

        codec.load(msg_in.GetDescriptor());
        codec.info(msg_in.GetDescriptor());

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

    // test non standard codec
    {
        TestMsg msg_in;
        msg_in.set_non_default_double(1200.56);
        msg_in.mutable_msg_oneof2()->set_val(100.123);

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

    // Test exception thrown for in_head oneof fields
    try
    {
        InvalidTestMsg msg_in;
        msg_in.set_double_oneof1(10.56);
        msg_in.mutable_msg_oneof2()->set_val(100.123);

        codec.load(msg_in.GetDescriptor());
        codec.info(msg_in.GetDescriptor());
    }
    catch (dccl::Exception& e)
    {
        // expect throw dccl::Exception with in_head == true
        assert(
            strcmp(e.what(),
                   "Oneof field used in header - oneof fields cannot be encoded in the header.") ==
            0);
    }
}
