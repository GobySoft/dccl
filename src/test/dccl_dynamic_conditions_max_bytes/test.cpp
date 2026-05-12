// Copyright 2023:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Community contributors (see AUTHORS file)
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
// tests dynamic max_bytes: messages can be sent over different channels
// with different byte-size constraints enforced at runtime.

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../../codec.h"
#include "test.pb.h"

using namespace dccl::test;

dccl::Codec codec;

// Test that a message with channel=WIFI (limit 200 bytes) encodes/decodes OK
// when data fits within 200 bytes.
void test_wifi_ok()
{
    codec.load<TestMaxBytes>();

    TestMaxBytes msg;
    msg.set_channel(TestMaxBytes::WIFI);
    // Add a few data values - well within 200 bytes
    for (int i = 0; i < 5; ++i)
        msg.add_data(i * 1000);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_wifi_ok: encoding..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "  encoded size: " << bytes.size() << " bytes" << std::endl;
    assert(bytes.size() <= 200);

    TestMaxBytes msg_out;
    codec.decode(bytes, &msg_out);

    assert(msg.SerializeAsString() == msg_out.SerializeAsString());
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_wifi_ok: passed" << std::endl;
}

// Test that a message with channel=ACOUSTIC (limit 10 bytes) encodes OK
// when data is small enough.
void test_acoustic_ok()
{
    codec.load<TestMaxBytes>();

    TestMaxBytes msg;
    msg.set_channel(TestMaxBytes::ACOUSTIC);
    // No extra data - just the channel field - should be tiny

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_acoustic_ok: encoding..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "  encoded size: " << bytes.size() << " bytes" << std::endl;
    assert(bytes.size() <= 10);

    TestMaxBytes msg_out;
    codec.decode(bytes, &msg_out);

    assert(msg.SerializeAsString() == msg_out.SerializeAsString());
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_acoustic_ok: passed" << std::endl;
}

// Test that a message with channel=ACOUSTIC (limit 10 bytes) throws when
// the encoded data exceeds 10 bytes.
void test_acoustic_too_large()
{
    codec.load<TestMaxBytes>();

    TestMaxBytes msg;
    msg.set_channel(TestMaxBytes::ACOUSTIC);
    // Add many data values to exceed 10 bytes
    for (int i = 0; i < 15; ++i)
        msg.add_data(i * 1000);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_acoustic_too_large: encoding (should throw)..." << std::endl;
    std::string bytes;
    bool threw = false;
    try
    {
        codec.encode(&bytes, msg);
    }
    catch (const dccl::Exception& e)
    {
        threw = true;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "  caught expected exception: " << e.what() << std::endl;
    }
    assert(threw && "Expected exception when encoding message exceeding dynamic max_bytes");
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "test_acoustic_too_large: passed" << std::endl;
}

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);

    test_wifi_ok();
    test_acoustic_ok();
    test_acoustic_too_large();

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
    return 0;
}
