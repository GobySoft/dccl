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
// tests all protobuf types with _default codecs, repeat and non repeat

#include <thread>

#include "dccl/arithmetic/field_codec_arithmetic.h"
#include "dccl/codec.h"
#include "test.pb.h"
#include "test_arithmetic.pb.h"

using namespace dccl::test;

void arithmetic_run_test(dccl::Codec& codec, dccl::arith::protobuf::ArithmeticModel& model,
                         const google::protobuf::Message& msg_in, bool set_model = true)
{
    if (set_model)
    {
        model.set_name("model");
        dccl::arith::ModelManager::set_model(codec, model);
    }

    codec.load(msg_in.GetDescriptor());
    codec.info(msg_in.GetDescriptor());

    //    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

    //    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    //    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    //    std::cout << "Try decode..." << std::endl;

    boost::shared_ptr<google::protobuf::Message> msg_out(msg_in.New());
    codec.decode(bytes, msg_out.get());

    //    std::cout << "... got Message out:\n" << msg_out->DebugString() << std::endl;

    assert(msg_in.SerializeAsString() == msg_out->SerializeAsString());
}

void decode_check(dccl::Codec& codec, const std::string& encoded, TestMsg msg_in);
void run(int thread, int num_iterations);
int main(int argc, char* argv[])
{
    {
        std::thread t1([]() { run(1, 100); });
        std::thread t2([]() { run(2, 100); });
        std::thread t3([]() { run(3, 100); });
        std::thread t4([]() { run(4, 100); });
        std::thread t5([]() { run(5, 100); });
        std::thread t6([]() { run(6, 100); });
        std::thread t7([]() { run(7, 100); });
        std::thread t8([]() { run(8, 100); });
        std::thread t9([]() { run(9, 100); });
        std::thread t10([]() { run(10, 100); });
        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
        t6.join();
        t7.join();
        t8.join();
        t9.join();
        t10.join();
    }

    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    {
        std::thread t1([]() { run(1, 10); });
        std::thread t2([]() { run(2, 10); });
        std::thread t3([]() { run(3, 10); });
        std::thread t4([]() { run(4, 10); });
        std::thread t5([]() { run(5, 10); });
        std::thread t6([]() { run(6, 10); });
        std::thread t7([]() { run(7, 10); });
        std::thread t8([]() { run(8, 10); });
        std::thread t9([]() { run(9, 10); });
        std::thread t10([]() { run(10, 10); });
        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
        t6.join();
        t7.join();
        t8.join();
        t9.join();
        t10.join();
    }

    std::cout << "all tests passed" << std::endl;
}

void run(int thread, int num_iterations)
{
    dccl::Codec codec;
    codec.load_library(DCCL_ARITHMETIC_NAME);
    codec.load<TestMsg>();
    for (int m = 0; m < num_iterations; ++m)
    {
        std::cout << "Thread " << thread << ", it: " << m << std::endl;
        codec.info<TestMsg>();
        TestMsg msg_in;
        int i = 0;
        msg_in.set_double_default_optional(++i + 0.1);
        msg_in.set_float_default_optional(++i + 0.2);

        msg_in.set_int32_default_optional(++i);
        msg_in.set_int64_default_optional(-++i);
        msg_in.set_uint32_default_optional(++i);
        msg_in.set_uint64_default_optional(++i);
        msg_in.set_sint32_default_optional(-++i);
        msg_in.set_sint64_default_optional(++i);
        msg_in.set_fixed32_default_optional(++i);
        msg_in.set_fixed64_default_optional(++i);
        msg_in.set_sfixed32_default_optional(++i);
        msg_in.set_sfixed64_default_optional(-++i);

        msg_in.set_bool_default_optional(true);

        msg_in.set_string_default_optional("abc123");
        msg_in.set_bytes_default_optional(dccl::hex_decode("00112233aabbcc1234"));

        msg_in.set_enum_default_optional(ENUM_C);
        msg_in.mutable_msg_default_optional()->set_val(++i + 0.3);
        msg_in.mutable_msg_default_optional()->mutable_msg()->set_val(++i);

        msg_in.set_double_default_required(++i + 0.1);
        msg_in.set_float_default_required(++i + 0.2);

        msg_in.set_int32_default_required(++i);
        msg_in.set_int64_default_required(-++i);
        msg_in.set_uint32_default_required(++i);
        msg_in.set_uint64_default_required(++i);
        msg_in.set_sint32_default_required(-++i);
        msg_in.set_sint64_default_required(++i);
        msg_in.set_fixed32_default_required(++i);
        msg_in.set_fixed64_default_required(++i);
        msg_in.set_sfixed32_default_required(++i);
        msg_in.set_sfixed64_default_required(-++i);

        msg_in.set_bool_default_required(true);

        msg_in.set_string_default_required("abc123");
        msg_in.set_bytes_default_required(dccl::hex_decode("00112233aabbcc1234"));

        msg_in.set_enum_default_required(ENUM_C);
        msg_in.mutable_msg_default_required()->set_val(++i + 0.3);
        msg_in.mutable_msg_default_required()->mutable_msg()->set_val(++i);

        for (int j = 0; j < 2; ++j)
        {
            msg_in.add_double_default_repeat(++i + 0.1);
            msg_in.add_float_default_repeat(++i + 0.2);

            msg_in.add_int32_default_repeat(++i);
            msg_in.add_int64_default_repeat(-++i);
            msg_in.add_uint32_default_repeat(++i);
            msg_in.add_uint64_default_repeat(++i);
            msg_in.add_sint32_default_repeat(-++i);
            msg_in.add_sint64_default_repeat(++i);
            msg_in.add_fixed32_default_repeat(++i);
            msg_in.add_fixed64_default_repeat(++i);
            msg_in.add_sfixed32_default_repeat(++i);
            msg_in.add_sfixed64_default_repeat(-++i);

            msg_in.add_bool_default_repeat(true);

            msg_in.add_string_default_repeat("abc123");

            if (j)
                msg_in.add_bytes_default_repeat(dccl::hex_decode("00aabbcc"));
            else
                msg_in.add_bytes_default_repeat(dccl::hex_decode("ffeedd12"));

            msg_in.add_enum_default_repeat(static_cast<Enum1>((++i % 3) + 1));
            EmbeddedMsg1* em_msg = msg_in.add_msg_default_repeat();
            em_msg->set_val(++i + 0.3);
            em_msg->mutable_msg()->set_val(++i);
        }

        std::string bytes;
        codec.encode(&bytes, msg_in);

        decode_check(codec, bytes, msg_in);

        // test case from Practical Implementations of Arithmetic Coding by Paul G. Howard and Je rey Scott Vitter
        {
            dccl::arith::protobuf::ArithmeticModel model;

            model.set_eof_frequency(4); // "a"

            model.add_value_bound(0);
            model.add_frequency(5); // "b"

            model.add_value_bound(1);
            model.add_frequency(1); // "EOF"

            model.add_value_bound(2);

            model.set_out_of_range_frequency(0);

            dccl::test::arith::ArithmeticDoubleTestMsg msg_in;

            msg_in.add_value(0); // b
            msg_in.add_value(0); // b
            msg_in.add_value(0); // b
            msg_in.add_value(1); // "EOF"

            arithmetic_run_test(codec, model, msg_in);
        }
    }
}

void decode_check(dccl::Codec& codec, const std::string& encoded, TestMsg msg_in)
{
    TestMsg msg_out;
    codec.decode(encoded, &msg_out);

    // std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

    // truncate to "max_length" as codec should do
    msg_in.set_string_default_repeat(0, "abc1");
    msg_in.set_string_default_repeat(1, "abc1");

    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
}
