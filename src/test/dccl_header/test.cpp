// Copyright 2011-2023:
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
#include "../../codecs2/field_codec_default.h"
#include "test.pb.h"

#include <sys/time.h>

#include "../../binary.h"
using namespace dccl::test;

template <dccl::int64 fixed_time_usec> struct DummyClock
{
    using time_point = std::chrono::time_point<std::chrono::system_clock>;
    static time_point now() { return time_point(std::chrono::microseconds(fixed_time_usec)); }
};

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    {
        dccl::Codec codec;
        GobyMessage msg_in1;

        msg_in1.set_telegram("hello!");

        timeval t;
        gettimeofday(&t, nullptr);
        dccl::int64 now = 1000000 * static_cast<dccl::int64>(t.tv_sec);

        msg_in1.mutable_header()->set_time(now);
        msg_in1.mutable_header()->set_time_signed(now);
        msg_in1.mutable_header()->set_time_double(now / 1000000);

        dccl::int64 past = now / 1000000.0 - 200000;   // Pick a time 2+ days in the past.
        dccl::int64 future = now / 1000000.0 + 200000; // Pick a time 2+ days in the future.
        msg_in1.mutable_header()->set_pasttime_double(past);
        msg_in1.mutable_header()->set_futuretime_double(future);

        dccl::int64 msec = t.tv_usec / 1000 * 1000;
        msg_in1.mutable_header()->set_time_precision(now + msec);
        msg_in1.mutable_header()->set_time_double_precision((now + t.tv_usec) / 1000000.0);

        msg_in1.mutable_header()->set_source_platform(1);
        msg_in1.mutable_header()->set_dest_platform(3);
        msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);
        msg_in1.set_const_int(3);

        codec.info(msg_in1.GetDescriptor(), &std::cout);
        std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
        codec.load(msg_in1.GetDescriptor());
        std::cout << "Try encode..." << std::endl;
        std::string bytes1;
        codec.encode(&bytes1, msg_in1);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes1) << std::endl;

        // test that adding garbage to the end does not affect decoding
        bytes1 += std::string(10, '\0');

        std::cout << "Try decode..." << std::endl;

        auto* msg_out1 = codec.decode<GobyMessage*>(bytes1);
        std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
        assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());
        delete msg_out1;
    }

    // test non-standard clock for time to allow post processing of dccl.time messages
    {
        dccl::Codec codec;
        // some fixed historic time
        constexpr dccl::int64 past_now = 1597743788000000;

        // received one second later
        dccl::v2::TimeCodecClock::set_clock<DummyClock<past_now + 1000000>>();

        TimeTest msg_in1;
        // some time in the past
        msg_in1.set_time(past_now);

        std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
        codec.load(msg_in1.GetDescriptor());
        std::cout << "Try encode..." << std::endl;
        std::string bytes1;
        codec.encode(&bytes1, msg_in1);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes1) << std::endl;

        std::cout << "Try decode..." << std::endl;
        auto msg_out1 = codec.decode<std::unique_ptr<google::protobuf::Message>>(bytes1);
        std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
        assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());
    }

    std::cout << "all tests passed" << std::endl;
}
