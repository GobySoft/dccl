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

void fill_message(NativeProtobufTest& msg_in)
{
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

    //    msg_in.set_enum_default_optional(ENUM_C);

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

    //    msg_in.set_enum_default_required(ENUM_C);
    
    
    for(int j = 0; j < 4; ++j)
    {
        msg_in.add_double_default_repeat(++i + 0.1);
        msg_in.add_int32_default_repeat(++i);
    }
}

void fill_message_partial(NativeProtobufTest& msg_in)
{
    int i = 0;
     msg_in.set_double_default_optional(++i + 0.1);
    msg_in.set_float_default_optional(++i + 0.2);

    msg_in.set_int32_default_optional(++i);
    msg_in.set_uint32_default_optional(++i);
    msg_in.set_uint64_default_optional(++i);
    msg_in.set_sint64_default_optional(++i);
    msg_in.set_fixed32_default_optional(++i);
    msg_in.set_sfixed32_default_optional(++i);
    msg_in.set_sfixed64_default_optional(-++i);


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

    //    msg_in.set_enum_default_required(ENUM_C);
    
    
    for(int j = 0; j < 2; ++j)
    {
        msg_in.add_double_default_repeat(++i + 0.1);
        msg_in.add_int32_default_repeat(++i);
    }
}


void fill_message_max(NativeProtobufTest& msg_in)
{
    msg_in.set_double_default_optional(std::numeric_limits<double>::max());
    msg_in.set_float_default_optional(std::numeric_limits<float>::max());

    msg_in.set_int32_default_optional(std::numeric_limits<dccl::int32>::max());
    msg_in.set_int64_default_optional(std::numeric_limits<dccl::int64>::max());
    msg_in.set_uint32_default_optional(std::numeric_limits<dccl::uint32>::max());
    msg_in.set_uint64_default_optional(std::numeric_limits<dccl::uint64>::max());
    msg_in.set_sint32_default_optional(std::numeric_limits<dccl::int32>::max());
    msg_in.set_sint64_default_optional(std::numeric_limits<dccl::int64>::max());
    msg_in.set_fixed32_default_optional(std::numeric_limits<dccl::uint32>::max());
    msg_in.set_fixed64_default_optional(std::numeric_limits<dccl::uint64>::max());
    msg_in.set_sfixed32_default_optional(std::numeric_limits<dccl::int32>::max());
    msg_in.set_sfixed64_default_optional(std::numeric_limits<dccl::int64>::max());

    msg_in.set_bool_default_optional(true);

    //    msg_in.set_enum_default_optional(ENUM_C);

    msg_in.set_double_default_required(std::numeric_limits<double>::max());
    msg_in.set_float_default_required(std::numeric_limits<float>::max());
    msg_in.set_int32_default_required(std::numeric_limits<dccl::int32>::max());
    msg_in.set_int64_default_required(std::numeric_limits<dccl::int64>::max());
    msg_in.set_uint32_default_required(std::numeric_limits<dccl::uint32>::max());
    msg_in.set_uint64_default_required(std::numeric_limits<dccl::uint64>::max());
    msg_in.set_sint32_default_required(std::numeric_limits<dccl::int32>::max());
    msg_in.set_sint64_default_required(std::numeric_limits<dccl::int64>::max());
    msg_in.set_fixed32_default_required(std::numeric_limits<dccl::uint32>::max());
    msg_in.set_fixed64_default_required(std::numeric_limits<dccl::uint64>::max());
    msg_in.set_sfixed32_default_required(std::numeric_limits<dccl::int32>::max());
    msg_in.set_sfixed64_default_required(std::numeric_limits<dccl::int64>::max());

    msg_in.set_bool_default_required(true);

    //    msg_in.set_enum_default_required(ENUM_C);
    
    
    for(int j = 0; j < 4; ++j)
    {
        msg_in.add_double_default_repeat(std::numeric_limits<double>::max());
        msg_in.add_int32_default_repeat(std::numeric_limits<dccl::int32>::max());
    }
}


void fill_message_min(NativeProtobufTest& msg_in)
{
    msg_in.set_double_default_optional(std::numeric_limits<double>::min());
    msg_in.set_float_default_optional(std::numeric_limits<float>::min());

    msg_in.set_int32_default_optional(std::numeric_limits<dccl::int32>::min());
    msg_in.set_int64_default_optional(std::numeric_limits<dccl::int64>::min());
    msg_in.set_uint32_default_optional(std::numeric_limits<dccl::uint32>::min());
    msg_in.set_uint64_default_optional(std::numeric_limits<dccl::uint64>::min());
    msg_in.set_sint32_default_optional(std::numeric_limits<dccl::int32>::min());
    msg_in.set_sint64_default_optional(std::numeric_limits<dccl::int64>::min());
    msg_in.set_fixed32_default_optional(std::numeric_limits<dccl::uint32>::min());
    msg_in.set_fixed64_default_optional(std::numeric_limits<dccl::uint64>::min());
    msg_in.set_sfixed32_default_optional(std::numeric_limits<dccl::int32>::min());
    msg_in.set_sfixed64_default_optional(std::numeric_limits<dccl::int64>::min());

    msg_in.set_bool_default_optional(true);

    //    msg_in.set_enum_default_optional(ENUM_C);

    msg_in.set_double_default_required(std::numeric_limits<double>::min());
    msg_in.set_float_default_required(std::numeric_limits<float>::min());
    msg_in.set_int32_default_required(std::numeric_limits<dccl::int32>::min());
    msg_in.set_int64_default_required(std::numeric_limits<dccl::int64>::min());
    msg_in.set_uint32_default_required(std::numeric_limits<dccl::uint32>::min());
    msg_in.set_uint64_default_required(std::numeric_limits<dccl::uint64>::min());
    msg_in.set_sint32_default_required(std::numeric_limits<dccl::int32>::min());
    msg_in.set_sint64_default_required(std::numeric_limits<dccl::int64>::min());
    msg_in.set_fixed32_default_required(std::numeric_limits<dccl::uint32>::min());
    msg_in.set_fixed64_default_required(std::numeric_limits<dccl::uint64>::min());
    msg_in.set_sfixed32_default_required(std::numeric_limits<dccl::int32>::min());
    msg_in.set_sfixed64_default_required(std::numeric_limits<dccl::int64>::min());

    msg_in.set_bool_default_required(true);

    //    msg_in.set_enum_default_required(ENUM_C);
    
    
    for(int j = 0; j < 4; ++j)
    {
        msg_in.add_double_default_repeat(std::numeric_limits<double>::min());
        msg_in.add_int32_default_repeat(std::numeric_limits<dccl::int32>::min());
    }
}


void run_test(dccl::Codec& codec, NativeProtobufTest& msg_in)
{
    NativeProtobufTest msg_out;
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec.encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes) << std::endl;
    std::cout << "Try decode..." << std::endl;
    codec.decode(bytes, &msg_out);
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;
    
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
}


int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);    

    dccl::Codec codec;
    codec.load_library(DCCL_NATIVE_PROTOBUF_NAME);

    codec.load<NativeProtobufTest>();
    codec.info<NativeProtobufTest>(&dccl::dlog);

    {
        NativeProtobufTest msg_in;
        fill_message(msg_in);
        run_test(codec, msg_in);
    }
    {
        NativeProtobufTest msg_in;
        fill_message_partial(msg_in);
        run_test(codec, msg_in);
    }
    {
        NativeProtobufTest msg_in;
        fill_message_min(msg_in);
        run_test(codec, msg_in);
    }
    
    
    {
        NativeProtobufTest msg_in;
        fill_message_max(msg_in);
        run_test(codec, msg_in);
    }
    
    
    
    std::cout << "all tests passed" << std::endl;
}

