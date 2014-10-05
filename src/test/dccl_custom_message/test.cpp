// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Applications
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.



// tests custom message codec
// tests cryptography

#include "dccl/codec.h"
#include "test.pb.h"


#include "dccl/binary.h"
using namespace dccl::test;

using dccl::operator<<;
using dccl::Bitset;

namespace dccl
{
    namespace test
    {
        class CustomCodec : public dccl::TypedFixedFieldCodec<CustomMsg>
        {
        private:
            unsigned size() { return (part() == dccl::HEAD) ? 0 : A_SIZE + B_SIZE; }
            Bitset encode() { return Bitset(size()); }
    
            Bitset encode(const CustomMsg& msg)
                {
                    if(part() == dccl::HEAD)
                    { return encode(); }
                    else
                    {
                        Bitset a(A_SIZE, static_cast<unsigned long>(msg.a()));
                        Bitset b(B_SIZE, static_cast<unsigned long>(msg.b()));
                
                        std::cout << "a: " << a << std::endl;
                        std::cout << "b: " << b  << std::endl;

                        a.append(b);
                        return a;
                    }
                }    
    
            CustomMsg decode(Bitset* bits)
                {
                    if(part() == dccl::HEAD)
                    { throw dccl::NullValueException(); }
                    else
                    {
                        Bitset a = *bits;
                        a.resize(A_SIZE);
                        Bitset b = *bits;
                        b >>= A_SIZE;
                        b.resize(B_SIZE);
                
                        CustomMsg msg;
                        msg.set_a(a.to_ulong());
                        msg.set_b(b.to_ulong());
                        return msg;
                    }
                }
    
    
            void validate() { }

            enum { A_SIZE = 32 };
            enum { B_SIZE = 1 };
        };    

        class Int32RepeatedCodec :
            public dccl::RepeatedTypedFieldCodec<dccl::int32>
        {
        private:
            enum { REPEAT_STORAGE_BITS = 4 };    
            enum { MAX_REPEAT_SIZE = 1 << REPEAT_STORAGE_BITS }; // 2^4

            dccl::int32 max() { return FieldCodecBase::dccl_field_options().max(); }
            dccl::int32 min() { return FieldCodecBase::dccl_field_options().min(); }
            dccl::int32 max_repeat() { return FieldCodecBase::dccl_field_options().max_repeat(); }
    
            Bitset encode_repeated(const std::vector<dccl::int32>& wire_values)
                {            
                    Bitset value_bits;
                    int repeat_size = static_cast<int>(wire_values.size()) > max_repeat() ?
                        max_repeat() :
                        wire_values.size();            

                    std::cout << "repeat size is " << repeat_size << std::endl;
            
                    for(int i = 0, n = repeat_size; i < n; ++i)
                    {
                        dccl::int32 wire_value = wire_values[i];
                        wire_value -= min();
                        value_bits.append(Bitset(singular_size(), static_cast<unsigned long>(wire_value)));
                    }

                    std::cout << value_bits << std::endl;
                    std::cout << Bitset(REPEAT_STORAGE_BITS, repeat_size) << std::endl;
                    Bitset out(REPEAT_STORAGE_BITS, repeat_size);
                    out.append(value_bits);
                    return out;
                }
    
            std::vector<dccl::int32> decode_repeated(Bitset* bits)
                {
                    int repeat_size = bits->to_ulong();
                    std::cout << "repeat size is " << repeat_size << std::endl;
                    // grabs more bits to add to the MSBs of `bits`
                    bits->get_more_bits(repeat_size*singular_size());

                    Bitset value_bits = *bits;
                    value_bits >>= REPEAT_STORAGE_BITS;

                    std::vector<dccl::int32> out;
                    for(int i = 0; i < repeat_size; ++i)
                    {
                        dccl::int32 value = value_bits.to_ulong() & ((1 << singular_size()) - 1);
                        value += min();
                        out.push_back(value);
                        value_bits >>= singular_size();
                    }
                    return out;
                }
    
            unsigned size_repeated(const std::vector<dccl::int32>& field_values)
                {
                    return REPEAT_STORAGE_BITS + field_values.size()*singular_size();
                }

            unsigned singular_size()
                { return dccl::ceil_log2((max()-min())+1); }
    
            unsigned max_size_repeated()
                {
                    return REPEAT_STORAGE_BITS + max_repeat()*singular_size();
                }
    
            unsigned min_size_repeated()
                {
                    return REPEAT_STORAGE_BITS;
                }

            void validate()
                {
                    FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_min(), "missing (dccl.field).min");
                    FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_max(), "missing (dccl.field).max");
                    FieldCodecBase::require(FieldCodecBase::dccl_field_options().max_repeat() < MAX_REPEAT_SIZE, "(dccl.field).max_repeat must be less than " + boost::lexical_cast<std::string>(static_cast<int>(MAX_REPEAT_SIZE)));
                }

    
        };

    }
}


    
int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;
    dccl::FieldCodecManager::add<dccl::test::CustomCodec>("custom_codec");
    dccl::FieldCodecManager::add<dccl::test::Int32RepeatedCodec>("int32_test_codec");
    
    codec.set_crypto_passphrase("my_passphrase!");

    CustomMsg msg_in1;

    msg_in1.set_a(10);
    msg_in1.set_b(true);
    
    codec.info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    codec.load(msg_in1.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes1;
    codec.encode(&bytes1, msg_in1);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;
    boost::shared_ptr<google::protobuf::Message> msg_out1 = codec.decode<boost::shared_ptr<google::protobuf::Message> >(bytes1);
    std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());


    CustomMsg2 msg_in2, msg_out2;

    msg_in2.mutable_msg()->CopyFrom(msg_in1);
    msg_in2.add_c(30);
    msg_in2.add_c(2);

    codec.info(msg_in2.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in2.DebugString() << std::endl;
    codec.load(msg_in2.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes2;
    codec.encode(&bytes2, msg_in2);
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes2) << std::endl;
    std::cout << "Try decode..." << std::endl;
    codec.decode(bytes2, &msg_out2);
    std::cout << "... got Message out:\n" << msg_out2.DebugString() << std::endl;
    assert(msg_in2.SerializeAsString() == msg_out2.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

