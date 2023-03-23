// Copyright 2012-2017:
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
// tests arithmetic encoder

#include <google/protobuf/descriptor.pb.h>

#include "dccl/arithmetic/field_codec_arithmetic.h"
#include "dccl/codec.h"

#include "test_arithmetic.pb.h"

#include "dccl/binary.h"
using namespace dccl::test::arith;

using dccl::operator<<;

void run_test(dccl::arith::protobuf::ArithmeticModel& model,
              const google::protobuf::Message& msg_in, bool set_model = true)
{
    static int i = 0;

    static dccl::Codec codec;

    if (!i)
    {
        void* dl_handle = dlopen(DCCL_ARITHMETIC_NAME, RTLD_LAZY);
        if (!dl_handle)
        {
            std::cerr << "Failed to open " << DCCL_ARITHMETIC_NAME << std::endl;
            exit(1);
        }
        codec.load_library(dl_handle);
    }

    if (set_model)
    {
        model.set_name("model");
        dccl::arith::ModelManager::set_model(codec, model);
    }

    codec.info(msg_in.GetDescriptor(), &std::cout);

    codec.load(msg_in.GetDescriptor());

    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;

    boost::shared_ptr<google::protobuf::Message> msg_out(msg_in.New());
    codec.decode(bytes, msg_out.get());

    std::cout << "... got Message out:\n" << msg_out->DebugString() << std::endl;

    assert(msg_in.SerializeAsString() == msg_out->SerializeAsString());
    ++i;
}

// usage: dccl_test10 [boolean: verbose]
int main(int argc, char* argv[])
{
    if (argc > 1 && std::string(argv[1]) == "1")
        dccl::dlog.connect(dccl::logger::DEBUG3_PLUS, &std::cerr);
    else
        dccl::dlog.connect(dccl::logger::DEBUG2_PLUS, &std::cerr);

    dccl::Codec codec;

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

        ArithmeticDoubleTestMsg msg_in;

        msg_in.add_value(0); // b
        msg_in.add_value(0); // b
        msg_in.add_value(0); // b
        msg_in.add_value(1); // "EOF"

        run_test(model, msg_in);
    }

    // misc test case
    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.add_value_bound(100.0);
        model.add_frequency(100);

        model.add_value_bound(100.1);
        model.add_frequency(100);

        model.add_value_bound(100.2);
        model.add_frequency(100);

        model.add_value_bound(100.3);
        model.add_frequency(100);

        model.add_value_bound(100.4);
        model.add_frequency(90);

        model.add_value_bound(100.5);
        model.add_frequency(125);

        model.add_value_bound(100.6);
        model.add_frequency(125);

        model.add_value_bound(100.7);
        model.add_frequency(125);

        model.add_value_bound(100.8);

        model.set_eof_frequency(25);
        model.set_out_of_range_frequency(10);

        ArithmeticDoubleTestMsg msg_in;

        msg_in.add_value(100.5);
        msg_in.add_value(100.7);
        msg_in.add_value(100.2);

        run_test(model, msg_in);
    }

    // edge case 1, should be just a single bit ("1")
    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.set_eof_frequency(10);
        model.set_out_of_range_frequency(0);

        model.add_value_bound(1);
        model.add_frequency(2);

        model.add_value_bound(2);
        model.add_frequency(3);

        model.add_value_bound(3);
        model.add_frequency(85);

        model.add_value_bound(4);

        ArithmeticEnumTestMsg msg_in;

        msg_in.add_value(ENUM_C);
        msg_in.add_value(ENUM_C);
        msg_in.add_value(ENUM_C);
        msg_in.add_value(ENUM_C);

        run_test(model, msg_in);
    }

    // edge case 2, should be full 23 or 24 bits
    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.set_eof_frequency(10);
        model.set_out_of_range_frequency(0);

        model.add_value_bound(1);
        model.add_frequency(2);

        model.add_value_bound(2);
        model.add_frequency(3);

        model.add_value_bound(3);
        model.add_frequency(85);

        model.add_value_bound(4);

        ArithmeticEnumTestMsg msg_in;

        msg_in.add_value(ENUM_A);
        msg_in.add_value(ENUM_A);
        msg_in.add_value(ENUM_A);
        msg_in.add_value(ENUM_A);

        run_test(model, msg_in);
    }

    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.set_eof_frequency(10);
        model.set_out_of_range_frequency(0);

        model.add_value_bound(1);
        model.add_frequency(2);

        model.add_value_bound(2);
        model.add_frequency(3);

        model.add_value_bound(3);
        model.add_frequency(85);

        model.add_value_bound(4);

        ArithmeticSingleEnumTestMsg msg_in;

        msg_in.set_value(ENUM_B);

        run_test(model, msg_in);
    }

    // test case from Practical Implementations of Arithmetic Coding by Paul G. Howard and Je rey Scott Vitter
    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.set_eof_frequency(1);

        model.add_value_bound(0);
        model.add_frequency(1);

        model.add_value_bound(1);
        model.add_frequency(1);

        model.add_value_bound(2);

        model.set_out_of_range_frequency(1);

        ArithmeticDouble3TestMsg msg_in;

        msg_in.add_value(0);
        msg_in.add_value(0);
        msg_in.add_value(0);
        msg_in.add_value(1);

        model.set_is_adaptive(true);
        run_test(model, msg_in);
        run_test(model, msg_in, false);
        run_test(model, msg_in, false);
        run_test(model, msg_in, false);
    }

    // test case from Arithmetic Coding revealed: A guided tour from theory to praxis Sable Technical Report No. 2007-5 Eric Bodden

    {
        dccl::arith::protobuf::ArithmeticModel model;

        model.set_eof_frequency(0);
        model.set_out_of_range_frequency(0);

        model.add_value_bound(1);
        model.add_frequency(2);

        model.add_value_bound(2);
        model.add_frequency(1);

        model.add_value_bound(3);
        model.add_frequency(3);

        model.add_value_bound(4);
        model.add_frequency(1);

        model.add_value_bound(5);
        model.add_frequency(1);

        model.add_value_bound(6);

        ArithmeticEnum2TestMsg msg_in;

        msg_in.add_value(ENUM2_A);
        msg_in.add_value(ENUM2_B);
        msg_in.add_value(ENUM2_C);
        msg_in.add_value(ENUM2_C);
        msg_in.add_value(ENUM2_E);
        msg_in.add_value(ENUM2_D);
        msg_in.add_value(ENUM2_A);
        msg_in.add_value(ENUM2_C);

        run_test(model, msg_in);
    }

    // randomly generate a model and a message
    // loop over all message lengths from 0 to 100
    srand(time(NULL));
    for (unsigned i = 0; i <= ArithmeticDouble2TestMsg::descriptor()
                                  ->FindFieldByName("value")
                                  ->options()
                                  .GetExtension(dccl::field)
                                  .max_repeat();
         ++i)
    {
        dccl::arith::protobuf::ArithmeticModel model;

        // pick some endpoints
        dccl::int32 low = -(rand() % std::numeric_limits<dccl::int32>::max());
        dccl::int32 high = rand() % std::numeric_limits<dccl::int32>::max();

        std::cout << "low: " << low << ", high: " << high << std::endl;

        // number of symbols
        dccl::int32 symbols = rand() % 1000 + 10;

        std::cout << "symbols: " << symbols << std::endl;

        // maximum freq
        dccl::arith::Model::freq_type each_max_freq =
            dccl::arith::Model::MAX_FREQUENCY / (symbols + 2);

        std::cout << "each_max_freq: " << each_max_freq << std::endl;

        model.set_eof_frequency(rand() % each_max_freq + 1);
        model.set_out_of_range_frequency(rand() % each_max_freq + 1);

        model.add_value_bound(low);
        model.add_frequency(rand() % each_max_freq + 1);
        for (int j = 1; j < symbols; ++j)
        {
            //            std::cout << "j: " << j << std::endl;

            dccl::int32 remaining_range = high - model.value_bound(j - 1);
            model.add_value_bound(model.value_bound(j - 1) +
                                  rand() % (remaining_range / symbols - j) + 1);
            model.add_frequency(rand() % each_max_freq + 1);
        }

        model.add_value_bound(high);

        ArithmeticDouble2TestMsg msg_in;

        for (unsigned j = 0; j < i; ++j) msg_in.add_value(model.value_bound(rand() % symbols));

        run_test(model, msg_in);

        std::cout << "end random test #" << i << std::endl;
    }

    std::cout << "all tests passed" << std::endl;
}
