// Copyright 2011-2022:
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
// tests all protobuf types with _default codecs, repeat and non repeat

#include <fstream>

#include <google/protobuf/descriptor.pb.h>

#include "dccl/codec.h"
#include "dccl/codecs3/field_codec_default.h"

#include "test.pb.h"
#include "dccl/binary.h"
using namespace dccl::test;

template <typename Msg>
void check(double val, bool should_pass);
dccl::Codec codec;
TestMsg msg_in;
TestMsgGroup msg_group_in;


namespace dccl
{
    namespace test
    {
        class TestCodec : public dccl::v3::DefaultNumericFieldCodec<double>
        {
            double max() { return 100; }
            double min() { return -100; }
            double precision() { return 1; }    // Deprecated
            double resolution() { return 0.1; }
            void validate() { }    
        };
    }
}

    


int main(int argc, char* argv[])
{
//    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    dccl::FieldCodecManager::add<dccl::test::TestCodec>("test.grouptest");
    dccl::FieldCodecManager::add<dccl::v3::DefaultMessageCodec, google::protobuf::FieldDescriptor::TYPE_MESSAGE>("test.grouptest");
    
    check<TestMsg>(50, true);
    check<TestMsg>(-50, false);
    check<TestMsgGroup>(50, true);
    check<TestMsgGroup>(-50, true);
    check<TestMsgVersion>(50, true);
    
    std::cout << "all tests passed" << std::endl;
}


template <typename Msg>
void check(double val, bool should_pass)
{
    Msg msg_in;
    int i = 0;
    msg_in.set_d(++i + 0.1);
    msg_in.add_d_repeat(12.1);
    msg_in.add_d_repeat(12.2);
    msg_in.add_d_repeat(12.3);
    
    msg_in.mutable_msg()->set_val(val);
    msg_in.mutable_msg()->mutable_msg()->set_val(val);
    codec.info(msg_in.GetDescriptor(), &std::cout);

    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
     
    codec.load(msg_in.GetDescriptor());

    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;
    
    Msg msg_out;
    codec.decode(bytes, &msg_out);
    
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

    if(should_pass)
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    else
        assert(msg_in.SerializeAsString() != msg_out.SerializeAsString());
}
