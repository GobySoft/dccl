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

#include <google/protobuf/descriptor.pb.h>

#include "dccl/codecs3/field_codec_default.h"
#include "dccl/codec.h"
#include "test.pb.h"
#include "dccl/binary.h"
using namespace dccl::test;

void decode_check(const std::string& encoded);

dccl::Codec codec;
TestMsgPack msg_pack;
TestMsgUnpack msg_unpack;

int main(int argc, char* argv[]) {
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    msg_pack.set_five_bit_padding(0);
    msg_pack.set_value(ENUM2_H);
    msg_unpack.set_value(ENUM2_H);

    codec.info(msg_pack.GetDescriptor());
    codec.info(msg_unpack.GetDescriptor());

    codec.load(msg_pack.GetDescriptor());
    codec.load(msg_unpack.GetDescriptor());

    std::cout << "Try encode..." << std::endl;
    std::string bytes_pack;
    codec.encode(&bytes_pack, msg_pack);
    std::cout << "... got packed bytes (hex): " << dccl::hex_encode(bytes_pack) << std::endl;

    std::string bytes_unpack;
    codec.encode(&bytes_unpack, msg_unpack);
    std::cout << "... got unpacked bytes (hex): " << dccl::hex_encode(bytes_unpack) << std::endl;

    std::cout << "Try decode..." << std::endl;
    TestMsgPack msg_pack_out;
    TestMsgUnpack msg_unpack_out;

    codec.decode(bytes_pack, &msg_pack_out);
    codec.decode(bytes_unpack, &msg_unpack_out);
    assert(msg_pack_out.SerializeAsString() == msg_pack.SerializeAsString());
    assert(msg_unpack_out.SerializeAsString() == msg_unpack.SerializeAsString());    
    std::cout << "all tests passed" << std::endl;
}
