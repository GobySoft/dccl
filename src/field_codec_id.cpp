// Copyright 2014-2017:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
#include "field_codec_id.h"

//
// DefaultIdentifierCodec
//

dccl::Bitset dccl::DefaultIdentifierCodec::encode() { return encode(0); }

dccl::Bitset dccl::DefaultIdentifierCodec::encode(const uint32& id)
{
    if (id <= ONE_BYTE_MAX_ID)
    {
        return (dccl::Bitset(this_size(id), id) << 1);
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

dccl::uint32 dccl::DefaultIdentifierCodec::decode(Bitset* bits)
{
    if (bits->test(0))
    {
        // long header
        // grabs more bits to add to the MSB of `bits`
        bits->get_more_bits((LONG_FORM_ID_BYTES - SHORT_FORM_ID_BYTES) * BITS_IN_BYTE);
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

unsigned dccl::DefaultIdentifierCodec::size() { return this_size(0); }

unsigned dccl::DefaultIdentifierCodec::size(const uint32& id) { return this_size(id); }

unsigned dccl::DefaultIdentifierCodec::this_size(const uint32& id)
{
    if (id > TWO_BYTE_MAX_ID)
        throw(Exception(
            "dccl.id provided (" + std::to_string(id) +
            ") exceeds maximum: " + std::to_string(int(TWO_BYTE_MAX_ID))));

    return (id <= ONE_BYTE_MAX_ID) ? SHORT_FORM_ID_BYTES * BITS_IN_BYTE
                                   : LONG_FORM_ID_BYTES * BITS_IN_BYTE;
}

unsigned dccl::DefaultIdentifierCodec::max_size() { return LONG_FORM_ID_BYTES * BITS_IN_BYTE; }

unsigned dccl::DefaultIdentifierCodec::min_size() { return SHORT_FORM_ID_BYTES * BITS_IN_BYTE; }
