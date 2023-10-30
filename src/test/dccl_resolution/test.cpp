// Copyright 2011-2023:
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
// tests bounds on DefaultNumericFieldCodec

#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    codec.load<NumericMsg>();
    codec.info<NumericMsg>(&dccl::dlog);

    try
    {
        codec.load<NegativeResolutionNumericMsg>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch (dccl::Exception& e)
    {
        std::cout
            << "** Note: this error is expected during proper execution of this unit test **: "
               "Field a failed validation: (dccl.field).resolution must be greater than 0."
            << std::endl;
    }

    try
    {
        codec.load<BothResolutionAndPrecisionSetNumericMsg>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch (dccl::Exception& e)
    {
        std::cout << "** Note: this error is expected during proper execution of this unit test "
                     "**: Field a failed validation: at most one of either (dccl.field).precision "
                     "or (dccl.field).resolution can be set."
                  << std::endl;
    }

    try
    {
        codec.load<TooBigNumericMsg>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch (dccl::Exception& e)
    {
        std::cout << "** Note: this error is expected during proper execution of this unit test "
                     "**: Field a failed validation: "
                     "[(dccl.field).max-(dccl.field).min]/(dccl.field).resolution must fit in a "
                     "double-precision floating point value. Please increase min, decrease max, or "
                     "decrease precision."
                  << std::endl;
    }

    try
    {
        codec.load<MinNotMultipleOfResolution>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch (dccl::Exception& e)
    {
        std::cout << "** Note: this error is expected during proper execution of this unit test "
                     "**: Field a failed validation: (dccl.field).min must be an exact multiple of "
                     "(dccl.field).resolution."
                  << std::endl;
    }

    try
    {
        codec.load<MaxNotMultipleOfResolution>();
        bool message_should_fail_load = false;
        assert(message_should_fail_load);
    }
    catch (dccl::Exception& e)
    {
        std::cout << "** Note: this error is expected during proper execution of this unit test "
                     "**: Field a failed validation: (dccl.field).max must be an exact multiple of "
                     "(dccl.field).resolution."
                  << std::endl;
    }

    NumericMsg msg_in;

    msg_in.set_a(10.12345678);
    msg_in.set_b(11.42106);
    msg_in.set_u1(18446744073709500000ull);
    msg_in.set_u2(0);
    msg_in.set_u3(10.2);
    msg_in.set_u4(5.6);
    msg_in.set_u5(1.95);
    msg_in.set_u6(25500);

    std::string encoded;
    codec.encode(&encoded, msg_in);

    NumericMsg msg_out;
    codec.decode(encoded, &msg_out);

    msg_in.set_b(11.4211);
    msg_in.set_u3(10.0);
    msg_in.set_u4(6);
    msg_in.set_u5(1.92);
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

    std::cout << "all tests passed" << std::endl;
}
