// Copyright 2011-2019:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
//   Kyle Guilbert <kguilbert@aphysci.com>
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
#ifndef DCCLFIELDCODECTYPED20120312H
#define DCCLFIELDCODECTYPED20120312H

#include <type_traits>

#include "field_codec.h"

namespace dccl
{

/// \brief A class that goes between FieldCodecBase and TypedFieldCodec to determine if the pre_encode() and post_decode() methods (which convert between WireType and FieldType) must be implemented or not.
template <typename WireType, typename FieldType, class Enable = void>
class FieldCodecSelector : public FieldCodecBase
{
  public:
    /// \brief Convert from the FieldType representation (used in the Google Protobuf message) to the WireType representation (used with encode() and decode(), i.e. "on the wire").
    ///
    /// \param field_value Value to convert
    /// \return Converted value
    virtual WireType pre_encode(const FieldType& field_value) = 0;

    /// \brief Convert from the WireType representation (used with encode() and decode(), i.e. "on the wire") to the FieldType representation (used in the Google Protobuf message).
    ///
    /// \param wire_value Value to convert
    /// \return Converted value
    virtual FieldType post_decode(const WireType& wire_value) = 0;
};

/// \brief A class that goes between FieldCodecBase and TypedFieldCodec to determine if the pre_encode() and post_decode() methods must be implemented or not. The specialization is selected if WireType == FieldType and implements these functions as a pass-through.
template <typename WireType, typename FieldType>
class FieldCodecSelector<WireType, FieldType,
                         std::enable_if_t<std::is_same<WireType, FieldType>::value>>
    : public FieldCodecBase
{
  public:
    /// \brief No-op version of pre_encode (since FieldType == WireType)
    virtual WireType pre_encode(const FieldType& field_value) { return field_value; }
    /// \brief No-op version of post_encode (since FieldType == WireType)
    virtual FieldType post_decode(const WireType& wire_value) { return wire_value; }
};

/// \brief Base class for static-typed (no dccl::any) field encoders/decoders. Most single-valued user defined variable length codecs will start with this class. Use TypedFixedFieldCodec if your codec is fixed length (always uses the same number of bits on the wire) or RepeatedTypedFieldCodec if your codec has special behavior for repeated (vector) fields.
///
/// \ingroup dccl_field_api
///\tparam WireType the type used for the encode and decode functions. This can be any C++ type, and is often the same as FieldType, unless a type conversion should be performed. The reason for using a different WireType and FieldType should be clear from the DefaultEnumCodec which uses DefaultNumericFieldCodec to do all the numerical encoding / decoding while DefaultEnumCodec does the type conversion (pre_encode() and post_decode()).
///\tparam FieldType the type used in the Google Protobuf message that is exposed to the end-user Codec::decode(), Codec::encode(), etc. functions.
template <typename WireType, typename FieldType = WireType>
class TypedFieldCodec : public FieldCodecSelector<WireType, FieldType>
{
  public:
    using wire_type = WireType;
    using field_type = FieldType;

  public:
    /// \brief Encode an empty field
    ///
    /// \return Bits represented the encoded field.
    virtual Bitset encode() = 0;

    /// \brief Encode a non-empty field
    ///
    /// \param wire_value Value to encode.
    /// \return Bits represented the encoded field.
    virtual Bitset encode(const WireType& wire_value) = 0;

    /// \brief Decode a field. If the field is empty (i.e. was encoded using the zero-argument encode()), throw NullValueException to indicate this.
    ///
    /// \param bits Bits to use for decoding.
    /// \return the decoded value.
    virtual WireType decode(Bitset* bits) = 0;

    /// \brief Calculate the size (in bits) of an empty field.
    ///
    /// \return the size (in bits) of the empty field.
    virtual unsigned size() = 0;

    /// \brief Calculate the size (in bits) of a non-empty field.
    ///
    /// \param wire_value Value to use when calculating the size of the field. If calculating the size requires encoding the field completely, cache the encoded value for a likely future call to encode() for the same wire_value.
    /// \return the size (in bits) of the field.
    virtual unsigned size(const WireType& wire_value) = 0;

  private:
    unsigned any_size(const dccl::any& wire_value) override
    {
        try
        {
            return is_empty(wire_value) ? size() : size(dccl::any_cast<WireType>(wire_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("size", typeid(WireType), wire_value.type()));
        }
    }

    void any_encode(Bitset* bits, const dccl::any& wire_value) override
    {
        try
        {
            *bits = is_empty(wire_value) ? encode() : encode(dccl::any_cast<WireType>(wire_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("encode", typeid(WireType), wire_value.type()));
        }
    }

    void any_decode(Bitset* bits, dccl::any* wire_value) override
    {
        any_decode_specific<WireType>(bits, wire_value);
    }

    void any_pre_encode(dccl::any* wire_value, const dccl::any& field_value) override
    {
        try
        {
            if (!is_empty(field_value))
                *wire_value = this->pre_encode(dccl::any_cast<FieldType>(field_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("pre_encode", typeid(FieldType), field_value.type()));
        }
        catch (NullValueException&)
        {
            *wire_value = dccl::any();
        }
    }

    void any_post_decode(const dccl::any& wire_value, dccl::any* field_value) override
    {
        any_post_decode_specific<WireType>(wire_value, field_value);
    }

    // we don't currently support type conversion (post_decode / pre_encode) of Message types
    template <typename T>
    typename std::enable_if_t<std::is_base_of<google::protobuf::Message, T>::value, void>
    any_post_decode_specific(const dccl::any& wire_value, dccl::any* field_value)
    {
        *field_value = wire_value;
    }

    template <typename T>
    typename std::enable_if_t<!std::is_base_of<google::protobuf::Message, T>::value, void>
    any_post_decode_specific(const dccl::any& wire_value, dccl::any* field_value)
    {
        try
        {
            if (!is_empty(wire_value))
                *field_value = this->post_decode(dccl::any_cast<WireType>(wire_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("post_decode", typeid(WireType), wire_value.type()));
        }
        catch (NullValueException&)
        {
            *field_value = dccl::any();
        }
    }

    template <typename T>
    typename std::enable_if_t<std::is_base_of<google::protobuf::Message, T>::value, void>
    any_decode_specific(Bitset* bits, dccl::any* wire_value)
    {
        try
        {
            auto* msg = dccl::any_cast<google::protobuf::Message*>(*wire_value);
            msg->CopyFrom(decode(bits));
        }
        catch (NullValueException&)
        {
            if (FieldCodecBase::this_field())
                *wire_value = dccl::any();
        }
    }

    template <typename T>
    typename std::enable_if_t<!std::is_base_of<google::protobuf::Message, T>::value, void>
    any_decode_specific(Bitset* bits, dccl::any* wire_value)
    {
        try
        {
            *wire_value = decode(bits);
        }
        catch (NullValueException&)
        {
            *wire_value = dccl::any();
        }
    }
};

/// \brief Base class for "repeated" (multiple value) static-typed (no dccl::any) field encoders/decoders. Use TypedFixedFieldCodec if your codec is fixed length (always uses the same number of bits on the wire). Use TypedFieldCodec if your fields are always singular ("optional" or "required"). Singular fields are default implemented in this codec by calls to the equivalent repeated function with an empty or single valued vector.
///
/// \ingroup dccl_field_api
///\tparam WireType the type used for the encode and decode functions. This can be any C++ type, and is often the same as FieldType, unless a type conversion should be performed. The reason for using a different WireType and FieldType should be clear from the DefaultEnumCodec which uses DefaultNumericFieldCodec to do all the numerical encoding / decoding while DefaultEnumCodec does the type conversion (pre_encode() and post_decode()).
///\tparam FieldType the type used in the Google Protobuf message that is exposed to the end-user Codec::decode(), Codec::encode(), etc. functions.
template <typename WireType, typename FieldType = WireType>
class RepeatedTypedFieldCodec : public TypedFieldCodec<WireType, FieldType>
{
  public:
    using wire_type = WireType;
    using field_type = FieldType;

  public:
    /// \brief Encode a repeated field
    virtual Bitset encode_repeated(const std::vector<WireType>& wire_value) = 0;

    /// \brief Decode a repeated field
    virtual std::vector<WireType> decode_repeated(Bitset* bits) = 0;

    /// \brief Give the size of a repeated field
    virtual unsigned size_repeated(const std::vector<WireType>& wire_values) = 0;

    /// \brief Give the max size of a repeated field
    virtual unsigned max_size_repeated() = 0;

    /// \brief Give the min size of a repeated field
    virtual unsigned min_size_repeated() = 0;

    /// \brief Encode an empty field
    ///
    /// \return Bits represented the encoded field.
    virtual Bitset encode() { return encode_repeated(std::vector<WireType>()); }

    /// \brief Encode a non-empty field
    ///
    /// \param wire_value Value to encode.
    /// \return Bits represented the encoded field.
    virtual Bitset encode(const WireType& wire_value)
    {
        return encode_repeated(std::vector<WireType>(1, wire_value));
    }

    /// \brief Decode a field. If the field is empty (i.e. was encoded using the zero-argument encode()), throw NullValueException to indicate this.
    ///
    /// \param bits Bits to use for decoding.
    /// \return the decoded value.
    virtual WireType decode(dccl::Bitset* bits)
    {
        std::vector<WireType> return_vec = decode_repeated(bits);
        if (is_empty(return_vec))
            throw dccl::NullValueException();
        else
            return return_vec.at(0);
    }

    /// \brief Calculate the size (in bits) of an empty field.
    ///
    /// \return the size (in bits) of the empty field.
    virtual unsigned size() { return size_repeated(std::vector<WireType>()); }

    /// \brief Calculate the size (in bits) of a non-empty field.
    ///
    /// \param wire_value Value to use when calculating the size of the field. If calculating the size requires encoding the field completely, cache the encoded value for a likely future call to encode() for the same wire_value.
    /// \return the size (in bits) of the field.
    virtual unsigned size(const WireType& wire_value)
    {
        return size_repeated(std::vector<WireType>(1, wire_value));
    }

    virtual unsigned max_size() { return max_size_repeated(); }

    virtual unsigned min_size() { return min_size_repeated(); }

  private:
    void any_encode_repeated(Bitset* bits, const std::vector<dccl::any>& wire_values)
    {
        try
        {
            std::vector<WireType> in;
            for (const auto& wire_value : wire_values)
            { in.push_back(dccl::any_cast<WireType>(wire_value)); } *bits = encode_repeated(in);
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("encode_repeated", typeid(WireType), wire_values.at(0).type()));
        }
    }

    void any_decode_repeated(Bitset* repeated_bits, std::vector<dccl::any>* field_values)
    {
        any_decode_repeated_specific<WireType>(repeated_bits, field_values);
    }

    template <typename T>
    typename std::enable_if_t<std::is_base_of<google::protobuf::Message, T>::value, void>
    any_decode_repeated_specific(Bitset* repeated_bits, std::vector<dccl::any>* wire_values)
    {
        std::vector<WireType> decoded_msgs = decode_repeated(repeated_bits);
        wire_values->resize(decoded_msgs.size(), WireType());

        for (int i = 0, n = decoded_msgs.size(); i < n; ++i)
        {
            auto* msg = dccl::any_cast<google::protobuf::Message*>(wire_values->at(i));
            msg->CopyFrom(decoded_msgs[i]);
        }
    }

    template <typename T>
    typename std::enable_if_t<!std::is_base_of<google::protobuf::Message, T>::value, void>
    any_decode_repeated_specific(Bitset* repeated_bits, std::vector<dccl::any>* wire_values)
    {
        std::vector<WireType> decoded = decode_repeated(repeated_bits);
        wire_values->resize(decoded.size(), WireType());

        for (int i = 0, n = decoded.size(); i < n; ++i) wire_values->at(i) = decoded[i];
    }

    void any_pre_encode(dccl::any* wire_value, const dccl::any& field_value)
    {
        try
        {
            if (!is_empty(field_value))
                *wire_value = this->pre_encode(dccl::any_cast<FieldType>(field_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("pre_encode", typeid(FieldType), field_value.type()));
        }
        catch (NullValueException&)
        {
            *wire_value = dccl::any();
        }
    }

    void any_post_decode(const dccl::any& wire_value, dccl::any* field_value)
    {
        try
        {
            if (!is_empty(wire_value))
                *field_value = this->post_decode(dccl::any_cast<WireType>(wire_value));
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("post_decode", typeid(WireType), wire_value.type()));
        }
        catch (NullValueException&)
        {
            *field_value = dccl::any();
        }
    }

    //          void any_pre_encode_repeated(std::vector<dccl::any>* wire_values,
    //                                       const std::vector<dccl::any>& field_values);

    //          void any_post_decode_repeated(const std::vector<dccl::any>& wire_values,
    //                                        std::vector<dccl::any>* field_values);

    unsigned any_size_repeated(const std::vector<dccl::any>& wire_values)
    {
        try
        {
            std::vector<WireType> in;
            for (const auto& wire_value : wire_values)
            { in.push_back(dccl::any_cast<WireType>(wire_value)); } return size_repeated(in);
        }
        catch (dccl::bad_any_cast&)
        {
            throw(type_error("size_repeated", typeid(WireType), wire_values.at(0).type()));
        }
    }
};

} // namespace dccl

#endif
