// Copyright 2014-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
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
#include "field_codec_default.h"

using namespace dccl::logger;

//
// DefaultStringCodec
//

dccl::Bitset dccl::v3::DefaultStringCodec::encode() { return Bitset(min_size()); }

dccl::Bitset dccl::v3::DefaultStringCodec::encode(const std::string& wire_value)
{
    std::string s = wire_value;
    if (s.size() > dccl_field_options().max_length())
    {
        if (this->strict())
            throw(dccl::OutOfRangeException(std::string("String too long for field: ") +
                                                FieldCodecBase::this_field()->DebugString(),
                                            this->this_field(), this->this_descriptor()));

        dccl::dlog.is(DEBUG2) &&
            dccl::dlog << "String " << s << " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(dccl_field_options().max_length());
    }

    Bitset value_bits;
    value_bits.from_byte_string(s);

    Bitset length_bits(min_size(), s.length());

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec value_bits: " << value_bits
                                        << std::endl;

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec length_bits: " << length_bits
                                        << std::endl;

    // adds to MSBs
    for (int i = 0, n = value_bits.size(); i < n; ++i) length_bits.push_back(value_bits[i]);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec created: " << length_bits
                                        << std::endl;

    return length_bits;
}

std::string dccl::v3::DefaultStringCodec::decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();

    if (value_length)
    {
        unsigned header_length = min_size();

        dccl::dlog.is(DEBUG2) && dccl::dlog << "Length of string is = " << value_length
                                            << std::endl;

        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits before get_more_bits " << *bits << std::endl;

        // grabs more bits to add to the MSBs of `bits`
        bits->get_more_bits(value_length * BITS_IN_BYTE);

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

unsigned dccl::v3::DefaultStringCodec::size() { return min_size(); }

unsigned dccl::v3::DefaultStringCodec::size(const std::string& wire_value)
{
    return std::min(min_size() + static_cast<unsigned>(wire_value.length() * BITS_IN_BYTE),
                    max_size());
}

unsigned dccl::v3::DefaultStringCodec::max_size()
{
    // string length + actual string
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned dccl::v3::DefaultStringCodec::min_size()
{
    return dccl::ceil_log2(dccl_field_options().max_length() + 1);
}

void dccl::v3::DefaultStringCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (dccl.field).max_length");
}

//
// DefaultEnumCodec
//
double dccl::v3::DefaultEnumCodec::max()
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();

    if (dccl_field_options().packed_enum())
    {
        return e->value_count() - 1;
    }
    else
    {
        const google::protobuf::EnumValueDescriptor* value = e->value(0);
        int32 maxVal = value->number();
        for (int i = 1; i < e->value_count(); ++i)
        {
            value = e->value(i);
            if (value->number() > maxVal)
            {
                maxVal = value->number();
            }
        }
        return maxVal;
    }
}

double dccl::v3::DefaultEnumCodec::min()
{
    if (dccl_field_options().packed_enum())
    {
        return 0;
    }
    else
    {
        const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
        const google::protobuf::EnumValueDescriptor* value = e->value(0);
        int32 minVal = value->number();
        for (int i = 1; i < e->value_count(); ++i)
        {
            value = e->value(i);
            if (value->number() < minVal)
            {
                minVal = value->number();
            }
        }
        return minVal;
    }
}

dccl::int32 dccl::v3::DefaultEnumCodec::pre_encode(
    const google::protobuf::EnumValueDescriptor* const& field_value)
{
    if (dccl_field_options().packed_enum())
        return field_value->index();
    else
        return field_value->number();
}

const google::protobuf::EnumValueDescriptor*
dccl::v3::DefaultEnumCodec::post_decode(const dccl::int32& wire_value)
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();

    if (dccl_field_options().packed_enum())
    {
        if (wire_value < e->value_count())
        {
            const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);
            return return_value;
        }
        else
            throw NullValueException();
    }
    else
    {
        const google::protobuf::EnumValueDescriptor* return_value =
            e->FindValueByNumber(wire_value);
        if (return_value != nullptr)
            return return_value;
        else
            throw NullValueException();
    }
}
