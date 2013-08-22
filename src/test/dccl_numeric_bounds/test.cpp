// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.



// tests bounds on DefaultNumericFieldCodec

#include "dccl/codec.h"
#include "test.pb.h"

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
    msg_in.set_b(11.4);

    std::string encoded;
    codec.encode(&encoded, msg_in);
    
    NumericMsg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    
    
    std::cout << "all tests passed" << std::endl;
}

