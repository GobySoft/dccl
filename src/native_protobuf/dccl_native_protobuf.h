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
#ifndef DCCL_NATIVE_PROTOBUF_20190218H
#define DCCL_NATIVE_PROTOBUF_20190218H

#include "dccl/field_codec_fixed.h"
#include "dccl/field_codec_typed.h"

namespace dccl
{
/// Implements the default Google Protocol Buffers encoder for a variety of numeric types
namespace native_protobuf
{

// https://developers.google.com/protocol-buffers/docs/encoding
// used for int32, int64, uint32, uint64, sint32, sint64, bool, enum
template<typename WireType>
class VarIntNumericFieldCodec : public TypedFieldCodec<WireType>
{
private:
    unsigned min_size()
        {
            return 8;
        }

    unsigned max_size()
        {
            return 8*9;
        }
    
    unsigned size() 
        {
            return min_size();
        }
    
    unsigned size(const WireType& wire_value)
        {
            return 0;
        }
    
    Bitset encode()
        {
            return Bitset();
        }
    
    Bitset encode(const WireType& wire_value)
        {
            return Bitset();
        }
    
    WireType decode(Bitset* bits)
        {
            return WireType();
        }

};

// used for fixed64, sfixed64, double
template<typename WireType>
class Fixed64BitFieldCodec : public TypedFixedFieldCodec<WireType>
{
private:
    unsigned size() 
        {
            return 64;
        }
    
    Bitset encode()
        {
            return Bitset();
        }
    
    Bitset encode(const WireType& wire_value)
        {
            return Bitset();
        }
    
    WireType decode(Bitset* bits)
        {
            return WireType();
        }
};

// used for fixed32, sfixed32, float
template<typename WireType>
class Fixed32BitFieldCodec : public TypedFixedFieldCodec<WireType>
{
    unsigned size() 
        {
            return 32;
        }
    
    Bitset encode()
        {
            return Bitset();
        }
    
    Bitset encode(const WireType& wire_value)
        {
            return Bitset();
        }
    
    WireType decode(Bitset* bits)
        {
            return WireType();
        }
};

}
}



#endif
