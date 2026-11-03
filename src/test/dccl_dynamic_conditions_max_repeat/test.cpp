// Copyright 2026:
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
// tests dynamic max_repeat: repeated fields whose maximum length depends on the
// message contents (e.g. the channel the message is sent over).

#include <cassert>
#include <iostream>
#include <string>

#include "../../codec.h"

#if CODEC_VERSION == 4
#include "test4.pb.h"
#elif CODEC_VERSION == 5
#include "test5.pb.h"
#endif

using namespace dccl::test;

dccl::Codec codec;

TestMsg make_msg(Channel channel, int num_behaviours, int num_values, int num_nested_values)
{
    TestMsg msg;
    msg.set_channel(channel);
    for (int i = 0; i < num_behaviours; ++i)
    {
        auto* b = msg.add_behaviours();
        if (i % 2)
        {
            b->mutable_transit()->set_x(i * 10);
            b->mutable_transit()->set_y(-i * 10);
        }
        else
        {
            b->mutable_loiter()->set_duration(i);
        }
    }
    for (int i = 0; i < num_values; ++i) msg.add_values(i);
    msg.set_trailer(123);
    msg.set_last(67);
    msg.mutable_local_channel()->set_channel(channel);

    if (num_nested_values >= 0)
    {
        for (int i = 0; i < num_nested_values; ++i) msg.mutable_nested()->add_values(i + 100);
        msg.mutable_nested()->set_trailer(45);
    }
    return msg;
}

std::string encode(const TestMsg& msg)
{
    std::string bytes;
    codec.encode(&bytes, msg);
    return bytes;
}

void check_roundtrip(const TestMsg& msg)
{
    std::string bytes = encode(msg);
    TestMsg msg_out;
    codec.decode(bytes, &msg_out);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "In: " << msg.ShortDebugString()
                                                    << "\nOut: " << msg_out.ShortDebugString()
                                                    << std::endl;
    assert(msg.SerializeAsString() == msg_out.SerializeAsString());
}

void test_within_limits()
{
    check_roundtrip(make_msg(ACOUSTIC, 3, 0, 2));
    check_roundtrip(make_msg(IRIDIUM, 10, 20, 8));
    // dynamic value above the static max_repeat is clamped to it
    check_roundtrip(make_msg(WIFI, 500, 20, 8));
    check_roundtrip(make_msg(WIFI, 0, 0, -1));
}

void test_local_channel()
{
    for (auto channel : {ACOUSTIC, WIFI})
    {
        auto msg = make_msg(channel, 1, 0, -1);
        auto* local = msg.mutable_local_channel();
        local->set_channel(ACOUSTIC);
        local->add_values(1);
        local->add_values(2);
        check_roundtrip(msg);
    }
}

void test_size_prefix()
{
    // the repeated size prefix only spans the dynamic range, so the same
    // contents are smaller on the acoustic channel
    // ACOUSTIC: behaviours ceil(log2(4)) = 2 bits, values 0 bits
    // WIFI: behaviours ceil(log2(501)) = 9 bits, values ceil(log2(21)) = 5 bits
    auto acoustic = make_msg(ACOUSTIC, 0, 0, -1);
    auto wifi = make_msg(WIFI, 0, 0, -1);
    unsigned acoustic_bits = codec.size(acoustic) * 8, wifi_bits = codec.size(wifi) * 8;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "ACOUSTIC: " << codec.size(acoustic)
                                                    << "B, WIFI: " << codec.size(wifi) << "B"
                                                    << std::endl;
    assert(acoustic_bits < wifi_bits);
    assert(encode(acoustic).size() == codec.size(acoustic));
    assert(encode(wifi).size() == codec.size(wifi));
}

void test_truncate()
{
    // non-strict: values beyond the dynamic max_repeat are dropped
    codec.set_strict(false);
    TestMsg msg_out;
    codec.decode(encode(make_msg(ACOUSTIC, 6, 5, 5)), &msg_out);
    assert(msg_out.behaviours_size() == 3);
    assert(msg_out.values_size() == 0);
    assert(msg_out.nested().values_size() == 2);
    assert(msg_out.trailer() == 123);
    assert(msg_out.nested().trailer() == 45);

    auto expected = make_msg(ACOUSTIC, 3, 0, 2);
    assert(msg_out.SerializeAsString() == expected.SerializeAsString());

    // strict: exceeding the dynamic max_repeat is an error
    codec.set_strict(true);
    bool threw = false;
    try
    {
        encode(make_msg(ACOUSTIC, 4, 0, -1));
    }
    catch (dccl::OutOfRangeException& e)
    {
        threw = true;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "caught expected exception: " << e.what()
                                                        << std::endl;
    }
    assert(threw);
    // but the same message is fine on Iridium
    check_roundtrip(make_msg(IRIDIUM, 4, 0, -1));
    codec.set_strict(false);
}

void test_v2_unsupported()
{
    bool threw = false;
    try
    {
        codec.load<TestMsgV2>();
    }
    catch (dccl::Exception& e)
    {
        threw = true;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "caught expected exception: " << e.what()
                                                        << std::endl;
    }
    assert(threw);
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

    codec.load<TestMsg>();
    codec.info<TestMsg>();

    test_within_limits();
    test_local_channel();
    test_size_prefix();
    test_truncate();
    test_v2_unsupported();

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
    return 0;
}
