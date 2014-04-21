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


// tests all protobuf types with _default codecs, repeat and non repeat

#include <fstream>

#include <google/protobuf/descriptor.pb.h>

#include "dccl/codec.h"
#include "test.pb.h"
#include "dccl/binary.h"

void decode_check(const std::string& encoded);
dccl::Codec codec;
TestMsg msg_in;

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

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

    
    for(int j = 0; j < 2; ++j)
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

        if(j)
            msg_in.add_bytes_default_repeat(dccl::hex_decode("00aabbcc"));
        else
            msg_in.add_bytes_default_repeat(dccl::hex_decode("ffeedd12"));
        
        msg_in.add_enum_default_repeat(static_cast<Enum1>((++i % 3) + 1));
        EmbeddedMsg1* em_msg = msg_in.add_msg_default_repeat();
        em_msg->set_val(++i + 0.3);
        em_msg->mutable_msg()->set_val(++i);
    }
    
    codec.info(msg_in.GetDescriptor());

    std::ofstream fout("/tmp/testmessage.pb");
    msg_in.SerializeToOstream(&fout);
    
    
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
     
    codec.load(msg_in.GetDescriptor());

    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;
    decode_check(bytes);

    // make sure DCCL defaults stay wire compatible

    // v3
    decode_check(dccl::hex_decode("047f277b16b95660c0b0188000d8c0132858800008000dc2c4c6626466024488cca8ee324bd05c3f23af0000ad9112a09509788013e0820b18e0005ed0204c6c2c4666062042644675975982c65235f10a006a8d53c44af975226109f452918d44318d64d258509f8cca54d3c926d44dcdf926154198d8580c4198d8588cfebb774b0450dd65d6aba754330440f5b7540b00"));    

    // run a bunch of tests with random strings
    std::string random = bytes;
    for(unsigned i = 0; i < 10; ++i)
    {    
        random[(rand() % (bytes.size()-1)+1)] = rand() % 256;
        std::cout << "Using junk bytes: " << dccl::hex_encode(random) << std::endl;
        
        try
        {
            TestMsg msg_out;
            codec.decode(random, &msg_out);
        }
        catch(dccl::Exception&)
        {
        }
    }
    
    std::cout << "all tests passed" << std::endl;
}


void decode_check(const std::string& encoded)
{
    TestMsg msg_out;
    codec.decode(encoded, &msg_out);
    
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

    // truncate to "max_length" as codec should do
    msg_in.set_string_default_repeat(0,"abc1");
    msg_in.set_string_default_repeat(1,"abc1");
    
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
}
