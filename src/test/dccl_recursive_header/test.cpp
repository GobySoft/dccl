// Copyright 2024:
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

// Tests that the codec recursively checks for in_head fields inside embedded
// messages.  In the issue scenario, Bar.foo does not have in_head=true, but
// Foo.header does.  Before the fix, Bar:foo::header was never encoded/decoded
// because the codec skipped foo (in_head=false) when processing the HEAD part.

#include <cassert>
#include <iostream>

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"

using namespace dccl::test;

template <typename BarMsg> void run_test(const std::string& label)
{
    dccl::Codec codec;
    BarMsg msg_in;

    msg_in.mutable_foo()->mutable_header()->set_field(42);
    msg_in.mutable_foo()->set_body_field(99);
    msg_in.set_bar_body(77);

    codec.load(msg_in.GetDescriptor());
    codec.info(msg_in.GetDescriptor(), &std::cout);

    std::cout << label << " message in:\n" << msg_in.DebugString() << std::endl;

    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << label << " encoded (hex): " << dccl::hex_encode(bytes) << std::endl;

    BarMsg msg_out;
    codec.decode(bytes, &msg_out);
    std::cout << label << " message out:\n" << msg_out.DebugString() << std::endl;

    // Verify that the header field (in_head) was correctly encoded and decoded
    assert(msg_out.foo().header().field() == msg_in.foo().header().field());
    // Verify that the body fields were also correctly handled
    assert(msg_out.foo().body_field() == msg_in.foo().body_field());
    assert(msg_out.bar_body() == msg_in.bar_body());
    // Full round-trip check
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

    std::cout << label << " passed.\n" << std::endl;
}

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    run_test<BarV3>("BarV3 (codec_version=3)");
    run_test<BarV4>("BarV4 (codec_version=5)");

    std::cout << "all tests passed" << std::endl;
    return 0;
}
