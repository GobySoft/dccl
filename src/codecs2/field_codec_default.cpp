// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
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

#include "dccl/codecs2/field_codec_default.h"
#include "dccl/codec.h"

using namespace dccl::logger;


//
// DefaultBoolCodec
//

dccl::Bitset dccl::v2::DefaultBoolCodec::encode()
{
    return Bitset(size());
}

dccl::Bitset dccl::v2::DefaultBoolCodec::encode(const bool& wire_value)
{
    return Bitset(size(), this_field()->is_required() ? wire_value : wire_value + 1);
}

bool dccl::v2::DefaultBoolCodec::decode(Bitset* bits)
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
        throw NullValueException();
    }
}


unsigned dccl::v2::DefaultBoolCodec::size()
{    
    // true and false
    const unsigned BOOL_VALUES = 2;
    // if field unspecified
    const unsigned NULL_VALUE = this_field()->is_required() ? 0 : 1;
    
    return dccl::ceil_log2(BOOL_VALUES + NULL_VALUE);
}

void dccl::v2::DefaultBoolCodec::validate()
{ }

//
// DefaultStringCodec
//

dccl::Bitset dccl::v2::DefaultStringCodec::encode()
{
    return Bitset(min_size());
}

dccl::Bitset dccl::v2::DefaultStringCodec::encode(const std::string& wire_value)
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

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec value_bits: " << value_bits << std::endl;    

    
    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec length_bits: " << length_bits << std::endl;    
    
    // adds to MSBs
    for(int i = 0, n = value_bits.size(); i < n; ++i)
        length_bits.push_back(value_bits[i]);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec created: " << length_bits << std::endl;
    
    
    return length_bits;
}

std::string dccl::v2::DefaultStringCodec::decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();
    
    if(value_length)
    {
        
        unsigned header_length = min_size();
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "Length of string is = " << value_length << std::endl;
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits before get_more_bits " << *bits << std::endl;    

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
        throw NullValueException();
    }
    
}

unsigned dccl::v2::DefaultStringCodec::size()
{
    return min_size();
}

unsigned dccl::v2::DefaultStringCodec::size(const std::string& wire_value)
{
    return std::min(min_size() + static_cast<unsigned>(wire_value.length()*BITS_IN_BYTE), max_size());
}


unsigned dccl::v2::DefaultStringCodec::max_size()
{
    // string length + actual string
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned dccl::v2::DefaultStringCodec::min_size()
{
    return dccl::ceil_log2(MAX_STRING_LENGTH+1);
}


void dccl::v2::DefaultStringCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (dccl.field).max_length");
    require(dccl_field_options().max_length() <= MAX_STRING_LENGTH,
            "(dccl.field).max_length must be <= " + boost::lexical_cast<std::string>(static_cast<int>(MAX_STRING_LENGTH)));
}

//
// DefaultBytesCodec
//
dccl::Bitset dccl::v2::DefaultBytesCodec::encode()
{
    return Bitset(min_size(), 0);
}


dccl::Bitset dccl::v2::DefaultBytesCodec::encode(const std::string& wire_value)
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

unsigned dccl::v2::DefaultBytesCodec::size()
{
    return min_size();    
}


unsigned dccl::v2::DefaultBytesCodec::size(const std::string& wire_value)
{
    return max_size();
}


std::string dccl::v2::DefaultBytesCodec::decode(Bitset* bits)
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
            throw NullValueException();
        }
    }
    else
    {
        return bits->to_byte_string();
    }
}

unsigned dccl::v2::DefaultBytesCodec::max_size()
{
    return dccl_field_options().max_length() * BITS_IN_BYTE +
        (this_field()->is_required() ? 0 : 1); // presence bit?
}

unsigned dccl::v2::DefaultBytesCodec::min_size()
{
    if(this_field()->is_required())
        return max_size();
    else
        return 1; // presence bit
}

void dccl::v2::DefaultBytesCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (dccl.field).max_length");
}

//
// DefaultEnumCodec
//
dccl::int32 dccl::v2::DefaultEnumCodec::pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
{
    return field_value->index();
}

const google::protobuf::EnumValueDescriptor* dccl::v2::DefaultEnumCodec::post_decode(const dccl::int32& wire_value)
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();

    if(wire_value < e->value_count())
    {
        const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);
        return return_value;
    }
    else
        throw NullValueException();
}



