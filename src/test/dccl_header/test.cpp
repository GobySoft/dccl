// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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



// tests proper encoding of standard Goby header

#include "dccl/dccl.h"
#include "dccl/dccl_field_codec_default.h"
#include "test.pb.h"


#include "dccl/binary.h"

using dccl::operator<<;



int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    dccl::DCCLModemIdConverterCodec::add("unicorn", 3);
    dccl::DCCLModemIdConverterCodec::add("topside", 1);
    
    
    dccl::Codec codec;

    GobyMessage msg_in1;

    msg_in1.set_telegram("hello!");

    timeval t;
    gettimeofday(&t, 0);
    dccl::int64 now = 1000000 * t.tv_sec;
    
    msg_in1.mutable_header()->set_time(now);
    msg_in1.mutable_header()->set_source_platform("topside");
    msg_in1.mutable_header()->set_dest_platform("unicorn");
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);
    
    codec.info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    codec.load(msg_in1.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes1;
    codec.encode(&bytes1, msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;

    // test that adding garbage to the end does not affect decoding
    bytes1 += std::string(10, '\0');
    
    std::cout << "Try decode..." << std::endl;
    
    GobyMessage* msg_out1 = codec.decode<GobyMessage*>(bytes1);
    std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());
    delete msg_out1;
    
    std::cout << "all tests passed" << std::endl;
}

