// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
// tests bounds on DefaultNumericFieldCodec

#include "dccl/codec.h"
#include "test.pb.h"
using namespace dccl::test;

using dccl::operator<<;

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    dccl::Codec codec;
    
    codec.load<NumericMsg>();
    codec.info<NumericMsg>(&dccl::dlog);

    try
    {
        codec.load<TooBigNumericMsg>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch(dccl::Exception& e)
    {
        std::cout << "** Note: this error is expected during proper execution of this unit test **: Field a failed validation: [(dccl.field).max-(dccl.field).min]*10^(dccl.field).precision must fit in a double-precision floating point value. Please increase min, decrease max, or decrease precision." << std::endl;
    }
    

    NumericMsg msg_in;

    msg_in.set_a(10.12345678);
    msg_in.set_b(11.42106);
    msg_in.set_u1(18446744073709500000ull);
    msg_in.set_u2(0);
    
    std::string encoded;
    codec.encode(&encoded, msg_in);
    
    NumericMsg msg_out;
    codec.decode(encoded, &msg_out);

    msg_in.set_b(11.4211);
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

    // Check negative precision encoding
    const int NUM_TESTS = 8;
    int test_values[NUM_TESTS][4] = {
      // a_set, a_result, b_set, b_result
      {20, 20, -500000, -500000},
      {0, 0, 254000, 254000},
      {10, 10, -257000, -257000},
      {-10, -10, -499000, -499000},
      {-20, -20, 500000, 500000},
      {-19, -20, 499999, 500000},
      {6, 10, -123400, -123000},
      {0, 0, 0, 0},
    };
    for (int i=0; i < NUM_TESTS; ++i) {
        NegativePrecisionNumericMsg msg_in_neg, msg_out_neg;
        std::string enc;
        msg_in_neg.set_a(test_values[i][0]);
        msg_in_neg.set_b(test_values[i][2]);

        codec.encode(&enc, msg_in_neg);
        codec.decode(enc, &msg_out_neg);
	
	std::cout << "msg_in: " << msg_in_neg.ShortDebugString() << std::endl;
	std::cout << "msg_out: " << msg_out_neg.ShortDebugString() << std::endl;

        assert(msg_out_neg.a() == test_values[i][1]);
        assert(msg_out_neg.b() == test_values[i][3]);
    }    
    
    std::cout << "all tests passed" << std::endl;
}

