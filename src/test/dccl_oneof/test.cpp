// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
// tests all protobuf types with _default codecs, repeat and non repeat

#include <fstream>

#include <google/protobuf/descriptor.pb.h>

#include "dccl/codec.h"
#include "test.pb.h"
#include "dccl/binary.h"
using namespace dccl::test;

dccl::Codec codec;
TestMsg msg_in;

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    msg_in.set_double_oneof1(10.56);
    msg_in.mutable_msg_oneof2()->set_val(100.123);

    try
    {
        codec.load(msg_in.GetDescriptor());
        codec.info(msg_in.GetDescriptor());
        
        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;
        
        std::cout << "Try decode..." << std::endl;
        
        TestMsg msg_out;
        codec.decode(bytes, &msg_out);
        
        std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;    
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    }
    catch(dccl::Exception& e)
    {
        // expect throw dccl::Exception in codec_version == 3
    }
    
}
