// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
// tests usage of a custom DCCL ID codec

#include "dccl/codec.h"
#include "dccl/field_codec.h"
#include "test.pb.h"
using namespace dccl::test;

using dccl::operator<<;

namespace dccl
{
    namespace test
    {    
        class MicroModemMiniPacketDCCLIDCodec : public dccl::TypedFixedFieldCodec<dccl::uint32>
        {
        private:
            dccl::Bitset encode(const dccl::uint32& wire_value);
    
            dccl::Bitset encode()
                { return encode(MINI_ID_OFFSET); }
    
            dccl::uint32 decode(dccl::Bitset* bits)
                { return bits->to_ulong() + MINI_ID_OFFSET; }
    
            unsigned size()
                { return MINI_ID_SIZE; }
    
            void validate()
                { }
    

            // Add this value when decoding to put us safely in our own namespace
            // from the normal default DCCL Codec
            enum { MINI_ID_OFFSET = 1000000 };    
            enum { MINI_ID_SIZE = 6 };
        };
    }
}


bool double_cmp(double a, double b, int precision)
{
    int a_whole = a;
    int b_whole = b;

    int a_part = (a-a_whole)*pow(10.0, precision);
    int b_part = (b-b_whole)*pow(10.0, precision);
    
    return (a_whole == b_whole) && (a_part == b_part);
}

dccl::Bitset dccl::test::MicroModemMiniPacketDCCLIDCodec::encode(const dccl::uint32& wire_value)
{
    // 16 bits, only 13 are useable, so
    // 3 "blank bits" + 3 bits for us
    return dccl::Bitset(MINI_ID_SIZE, wire_value - MINI_ID_OFFSET);
}


int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    {        
	dccl::FieldCodecManager::add<dccl::test::MicroModemMiniPacketDCCLIDCodec>("mini_id_codec");
        dccl::Codec codec("mini_id_codec");
        codec.set_crypto_passphrase("309ldskjfla39");
        
        codec.load<MiniUser>();
        codec.info<MiniUser>(&dccl::dlog);	  
        
        MiniUser mini_user_msg_in, mini_user_msg_out;
        mini_user_msg_in.set_user(876);
        std::string encoded;
        codec.encode(&encoded, mini_user_msg_in);
        codec.decode(encoded, &mini_user_msg_out);
        assert(mini_user_msg_out.SerializeAsString() == mini_user_msg_in.SerializeAsString());
        
        codec.load<MiniOWTT>();
        codec.info<MiniOWTT>(&dccl::dlog);
        
        MiniOWTT mini_owtt_in, mini_owtt_out; 
        mini_owtt_in.set_clock_mode(3);
        mini_owtt_in.set_tod(12);
        mini_owtt_in.set_user(13);

        encoded.clear();
        codec.encode(&encoded, mini_owtt_in);
        std::cout << "OWTT as hex: " << dccl::hex_encode(encoded) << std::endl;
        
        codec.decode(encoded, &mini_owtt_out);
        assert(mini_owtt_out.SerializeAsString() == mini_owtt_in.SerializeAsString());
        
        codec.load<MiniAbort>();
        codec.info<MiniAbort>(&dccl::dlog);
        
        MiniAbort mini_abort_in, mini_abort_out; 
        mini_abort_in.set_user(130);
        encoded.clear();
        codec.encode(&encoded, mini_abort_in);
        codec.decode(encoded, &mini_abort_out);
        assert(mini_abort_out.SerializeAsString() == mini_abort_in.SerializeAsString());
    }

    std::cout << "all tests passed" << std::endl;

}

