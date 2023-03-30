// Copyright 2011-2017:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
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
// tests proper encoding of standard Goby header

#include "../../codec.h"
#include "test1.pb.h"
#include "test2.pb.h"

#include <sys/time.h>

#include "../../binary.h"
using namespace dccl::test;

template <typename Message> void run_test(dccl::Codec& codec, Message& msg_in)
{
    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;

    auto msg_out = codec.decode<std::unique_ptr<google::protobuf::Message>>(bytes);
    std::cout << "... got Message out:\n" << msg_out->DebugString() << std::endl;
    assert(msg_in.SerializeAsString() == msg_out->SerializeAsString());
}

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;
    codec.load_library("libtest_autoload" SHARED_LIBRARY_SUFFIX);

    codec.info_all(&std::cout);

    TestMessage1 msg_in1;
    msg_in1.set_a(10);
    run_test(codec, msg_in1);

    TestMessage2 msg_in2;
    msg_in2.set_b(5);
    run_test(codec, msg_in2);

    TestMessage3 msg_in3;
    msg_in3.set_c(53);
    run_test(codec, msg_in3);

    TestMessage3SupersetName msg_in4;
    msg_in4.set_d(12);
    run_test(codec, msg_in4);

    std::cout << "all tests passed" << std::endl;
}
