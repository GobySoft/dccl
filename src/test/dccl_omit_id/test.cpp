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

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

dccl::Codec codec;

template <typename TestMsg> void decode_check(const TestMsg& msg_in)
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

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    codec.load<TestMsgOmit1>();
    codec.info<TestMsgOmit1>();

    codec.load<TestMsgOmit2>();
    codec.info<TestMsgOmit2>();

    codec.load<TestMsgNormal>();
    codec.info<TestMsgNormal>();

    codec.info_all();

    {
        TestMsgOmit1 msg_in;
        msg_in.set_d(10.0);
        msg_in.set_i(1000);

        decode_check(msg_in);
    }

    {
        TestMsgOmit2 msg_in;
        msg_in.set_u(15);

        decode_check(msg_in);
    }

    {
        TestMsgNormal msg_in;
        msg_in.set_f(2.3);

        decode_check(msg_in);
    }

    {
        TestMsgNormal msg_in;
        msg_in.set_f(2.3);

        std::string bytes;
        codec.encode(&bytes, msg_in);
        TestMsgNormalId3 msg_out;

        // mismatch in ID used to encode vs decode
        try
        {
            codec.decode(bytes, &msg_out);
        }
        catch (const dccl::Exception& e)
        {
            std::cout << "Caught expected exception: " << e.what() << std::endl;
        }
    }

    std::cout << "all tests passed" << std::endl;
}
