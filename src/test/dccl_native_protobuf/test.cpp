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
// tests usage of Legacy CCL

#include "test.pb.h"
#include "dccl.h"

using namespace dccl::test;

using dccl::operator<<;


int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);    

    dccl::Codec codec;
    codec.load_library(DCCL_NATIVE_PROTOBUF_NAME);

    codec.load<NativeProtobufTest>();
    codec.info<NativeProtobufTest>(&dccl::dlog);

    NativeProtobufTest msg_in, msg_out;
    msg_in.set_a_uint32(25);
    
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;
    std::cout << "Try decode..." << std::endl;
    codec.decode(bytes, &msg_out);
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
    
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

    
    std::cout << "all tests passed" << std::endl;
}

