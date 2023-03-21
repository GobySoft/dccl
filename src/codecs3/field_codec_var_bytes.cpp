// Copyright 2018-2019:
//   GobySoft, LLC (2013-)
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
#include "field_codec_var_bytes.h"
#include "dccl/codecs3/field_codec_default.h"
#include "dccl/field_codec_manager.h"

using namespace dccl::logger;

dccl::Bitset dccl::v3::VarBytesCodec::encode() { return dccl::Bitset(min_size()); }

dccl::Bitset dccl::v3::VarBytesCodec::encode(const std::string& wire_value)
{
    std::string s = wire_value;
    if (s.size() > dccl_field_options().max_length())
    {
        if (this->strict())
            throw(dccl::OutOfRangeException(std::string("Bytes too long for field: ") +
                                                FieldCodecBase::this_field()->DebugString(),
                                            this->this_field()));

        dccl::dlog.is(DEBUG2) &&
            dccl::dlog << "Bytes " << s << " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(dccl_field_options().max_length());
    }

    dccl::Bitset value_bits;
    value_bits.from_byte_string(s);

    dccl::Bitset length_bits(presence_size() + prefix_size(), s.length());

    if (!use_required()) // set the presence bit
    {
        length_bits <<= 1;
        length_bits.set(0);
    }

    dccl::dlog.is(DEBUG2) && dccl::dlog << "dccl::v3::VarBytesCodec value_bits: " << value_bits
                                        << std::endl;

    dccl::dlog.is(DEBUG2) && dccl::dlog << "dccl::v3::VarBytesCodec length_bits: " << length_bits
                                        << std::endl;

    // adds to MSBs
    for (int i = 0, n = value_bits.size(); i < n; ++i) length_bits.push_back(value_bits[i]);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "dccl::v3::VarBytesCodec created: " << length_bits
                                        << std::endl;

    return length_bits;
}

std::string dccl::v3::VarBytesCodec::decode(dccl::Bitset* bits)
{
    if (!use_required())
    {
        if (bits->to_ulong() == 0)
        {
            throw dccl::NullValueException();
        }
        else
        {
            bits->get_more_bits(prefix_size());
            (*bits) >>= 1;
        }
    }

    unsigned value_length = bits->to_ulong();
    unsigned header_length = presence_size() + prefix_size();

    dccl::dlog.is(DEBUG2) && dccl::dlog << "Length of string is = " << value_length << std::endl;

    dccl::dlog.is(DEBUG2) && dccl::dlog << "bits before get_more_bits " << *bits << std::endl;

    // grabs more bits to add to the MSBs of `bits`
    bits->get_more_bits(value_length * dccl::BITS_IN_BYTE);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "bits after get_more_bits " << *bits << std::endl;
    dccl::Bitset string_body_bits = *bits;
    string_body_bits >>= header_length;
    string_body_bits.resize(bits->size() - header_length);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "string_body_bits " << string_body_bits << std::endl;

    return string_body_bits.to_byte_string();
}

unsigned dccl::v3::VarBytesCodec::size() { return min_size(); }

unsigned dccl::v3::VarBytesCodec::size(const std::string& wire_value)
{
    return std::min(presence_size() + prefix_size() +
                        static_cast<unsigned>(wire_value.length() * dccl::BITS_IN_BYTE),
                    max_size());
}

unsigned dccl::v3::VarBytesCodec::max_size()
{
    return presence_size() + prefix_size() + dccl_field_options().max_length() * dccl::BITS_IN_BYTE;
}

unsigned dccl::v3::VarBytesCodec::min_size()
{
    if (use_required())
        return prefix_size();
    else
        return presence_size();
}

void dccl::v3::VarBytesCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (dccl.field).max_length");
}
