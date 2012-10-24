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




#include <sstream>
#include <algorithm>

#include "field_codec_default.h"
#include "type_helper.h"
#include "dccl/codec.h"

using namespace dccl::logger;


//
// DCCLDefaultIdentifierCodec
//

dccl::Bitset dccl::DCCLDefaultIdentifierCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::DCCLDefaultIdentifierCodec::encode(const uint32& id)
{
    if(id <= ONE_BYTE_MAX_ID)
    {
        return(dccl::Bitset(this_size(id), id) << 1);
    }
    else
    {
        dccl::Bitset return_bits(this_size(id), id);
        return_bits <<= 1;
        // set LSB to indicate long header form
        return_bits.set(0, true);

        
        return return_bits;
    }
}

dccl::uint32 dccl::DCCLDefaultIdentifierCodec::decode(Bitset* bits)
{
    if(bits->test(0))
    {
        // long header
        // grabs more bits to add to the MSB of `bits`
        bits->get_more_bits((LONG_FORM_ID_BYTES - SHORT_FORM_ID_BYTES)*BITS_IN_BYTE);
        // discard identifier
        *(bits) >>= 1;
        return bits->to_ulong();
    }
    else
    {
        // short header
        *(bits) >>= 1;
        return bits->to_ulong();
    }
}

unsigned dccl::DCCLDefaultIdentifierCodec::size()
{
    return this_size(0);
}

unsigned dccl::DCCLDefaultIdentifierCodec::size(const uint32& id)
{
    return this_size(id);
}

unsigned dccl::DCCLDefaultIdentifierCodec::this_size(const uint32& id)
{
    if(id < 0 || id > TWO_BYTE_MAX_ID)
        throw(Exception("dccl.id provided (" + boost::lexical_cast<std::string>(id) + ") is less than 0 or exceeds maximum: " + boost::lexical_cast<std::string>(int(TWO_BYTE_MAX_ID))));
    
    return (id <= ONE_BYTE_MAX_ID) ?
        SHORT_FORM_ID_BYTES*BITS_IN_BYTE :
        LONG_FORM_ID_BYTES*BITS_IN_BYTE;
}


unsigned dccl::DCCLDefaultIdentifierCodec::max_size()
{
    return LONG_FORM_ID_BYTES * BITS_IN_BYTE;
}

unsigned dccl::DCCLDefaultIdentifierCodec::min_size()
{
    return SHORT_FORM_ID_BYTES * BITS_IN_BYTE;
}


        

//
// DCCLDefaultBoolCodec
//

dccl::Bitset dccl::DCCLDefaultBoolCodec::encode()
{
    return Bitset(size());
}

dccl::Bitset dccl::DCCLDefaultBoolCodec::encode(const bool& wire_value)
{
    return Bitset(size(), this_field()->is_required() ? wire_value : wire_value + 1);
}

bool dccl::DCCLDefaultBoolCodec::decode(Bitset* bits)
{
    unsigned long t = bits->to_ulong();
    if(this_field()->is_required())
    {
        return t;
    }
    else if(t)
    {
        --t;
        return t;
    }
    else
    {
        throw(NullValueException());
    }
}


unsigned dccl::DCCLDefaultBoolCodec::size()
{    
    // true and false
    const unsigned BOOL_VALUES = 2;
    // if field unspecified
    const unsigned NULL_VALUE = this_field()->is_required() ? 0 : 1;
    
    return goby::util::ceil_log2(BOOL_VALUES + NULL_VALUE);
}

void dccl::DCCLDefaultBoolCodec::validate()
{ }

//
// DCCLDefaultStringCodec
//

dccl::Bitset dccl::DCCLDefaultStringCodec::encode()
{
    return Bitset(min_size());
}

dccl::Bitset dccl::DCCLDefaultStringCodec::encode(const std::string& wire_value)
{
    std::string s = wire_value;
    if(s.size() > dccl_field_options().max_length())
    {
        dccl::dlog.is(DEBUG2) && dccl::dlog << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(dccl_field_options().max_length()); 
    }
        
            
    Bitset value_bits;
    value_bits.from_byte_string(s);
    
    Bitset length_bits(min_size(), s.length());

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DCCLDefaultStringCodec value_bits: " << value_bits << std::endl;    

    
    dccl::dlog.is(DEBUG2) && dccl::dlog << "DCCLDefaultStringCodec length_bits: " << length_bits << std::endl;    
    
    // adds to MSBs
    for(int i = 0, n = value_bits.size(); i < n; ++i)
        length_bits.push_back(value_bits[i]);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DCCLDefaultStringCodec created: " << length_bits << std::endl;
    
    
    return length_bits;
}

std::string dccl::DCCLDefaultStringCodec::decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();
    
    if(value_length)
    {
        
        unsigned header_length = min_size();
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "Length of string is = " << value_length << std::endl;
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits before get_more_bits " << bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        bits->get_more_bits(value_length*BITS_IN_BYTE);

        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits after get_more_bits " << *bits << std::endl;    
        Bitset string_body_bits = *bits;
        string_body_bits >>= header_length;
        string_body_bits.resize(bits->size() - header_length);
    
        return string_body_bits.to_byte_string();
    }
    else
    {
        throw(NullValueException());
    }
    
}

unsigned dccl::DCCLDefaultStringCodec::size()
{
    return min_size();
}

unsigned dccl::DCCLDefaultStringCodec::size(const std::string& wire_value)
{
    return std::min(min_size() + static_cast<unsigned>(wire_value.length()*BITS_IN_BYTE), max_size());
}


unsigned dccl::DCCLDefaultStringCodec::max_size()
{
    // string length + actual string
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned dccl::DCCLDefaultStringCodec::min_size()
{
    return goby::util::ceil_log2(MAX_STRING_LENGTH+1);
}


void dccl::DCCLDefaultStringCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (goby.field).dccl.max_length");
    require(dccl_field_options().max_length() <= MAX_STRING_LENGTH,
            "(goby.field).dccl.max_length must be <= " + boost::lexical_cast<std::string>(static_cast<int>(MAX_STRING_LENGTH)));
}

//
// DCCLDefaultBytesCodec
//
dccl::Bitset dccl::DCCLDefaultBytesCodec::encode()
{
    return Bitset(min_size(), 0);
}


dccl::Bitset dccl::DCCLDefaultBytesCodec::encode(const std::string& wire_value)
{
    Bitset bits;
    bits.from_byte_string(wire_value);
    bits.resize(max_size());
    
    if(!this_field()->is_required())
    {
        bits <<= 1;
        bits.set(0, true); // presence bit
    }
    
    return bits;
}

unsigned dccl::DCCLDefaultBytesCodec::size()
{
    return min_size();    
}


unsigned dccl::DCCLDefaultBytesCodec::size(const std::string& wire_value)
{
    return max_size();
}


std::string dccl::DCCLDefaultBytesCodec::decode(Bitset* bits)
{
    if(!this_field()->is_required())
    {
        if(bits->to_ulong())
        {
            // grabs more bits to add to the MSBs of `bits`
            bits->get_more_bits(max_size()- min_size());
            
            Bitset bytes_body_bits = *bits;
            bytes_body_bits >>= min_size();
            bytes_body_bits.resize(bits->size() - min_size());
        
            return bytes_body_bits.to_byte_string();
        }
        else
        {
            throw(NullValueException());
        }
    }
    else
    {
        return bits->to_byte_string();
    }
}

unsigned dccl::DCCLDefaultBytesCodec::max_size()
{
    return dccl_field_options().max_length() * BITS_IN_BYTE +
        (this_field()->is_required() ? 0 : 1); // presence bit?
}

unsigned dccl::DCCLDefaultBytesCodec::min_size()
{
    if(this_field()->is_required())
        return max_size();
    else
        return 1; // presence bit
}

void dccl::DCCLDefaultBytesCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (goby.field).dccl.max_length");
}

//
// DCCLDefaultEnumCodec
//
dccl::int32 dccl::DCCLDefaultEnumCodec::pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
{
    return field_value->index();
}

const google::protobuf::EnumValueDescriptor* dccl::DCCLDefaultEnumCodec::post_decode(const dccl::int32& wire_value)
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
    const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);

    if(return_value)
        return return_value;
    else
        throw(NullValueException());
}



//
// DCCLModemIdConverterCodec
//

boost::bimap<std::string, dccl::int32> dccl::DCCLModemIdConverterCodec::platform2modem_id_;

dccl::int32 dccl::DCCLModemIdConverterCodec::pre_encode(const std::string& field_value)
{
    int32 v = BROADCAST_ID;
    if(platform2modem_id_.left.count(boost::to_lower_copy(field_value)))
        v = platform2modem_id_.left.at(field_value);
    
    return v;
}
            
std::string dccl::DCCLModemIdConverterCodec::post_decode(const int32& wire_value)
{
    if(wire_value == BROADCAST_ID)
        return "broadcast";
    else if(platform2modem_id_.right.count(wire_value))
        return platform2modem_id_.right.at(wire_value);
    else
        throw NullValueException();
}            


