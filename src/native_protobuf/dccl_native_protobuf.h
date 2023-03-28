// Copyright 2019:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Russ <russ@rw.id.au>
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

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format_lite.h>

#include <google/protobuf/stubs/common.h>
#if GOOGLE_PROTOBUF_VERSION < 3008000
#include <google/protobuf/wire_format_lite_inl.h> // this .h has been removed in protobuf 3.8
#endif

#include "../field_codec_fixed.h"
#include "../field_codec_typed.h"

namespace dccl
{
/// Implements the default Google Protocol Buffers encoder for a variety of numeric types
namespace native_protobuf
{
template <typename WireType, google::protobuf::internal::WireFormatLite::FieldType DeclaredType>
struct PrimitiveTypeHelperBase
{
    WireType decode(google::protobuf::io::CodedInputStream* input_stream)
    {
        WireType value;
        google::protobuf::internal::WireFormatLite::ReadPrimitive<WireType, DeclaredType>(
            input_stream, &value);
        return value;
    }
};

template <typename WireType, google::protobuf::FieldDescriptor::Type DeclaredType>
struct PrimitiveTypeHelper
{
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_INT64>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_INT64>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::Int64Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteInt64NoTagToArray(wire_value,
                                                                           &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_INT32>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_INT32>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::Int32Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteInt32NoTagToArray(wire_value,
                                                                           &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_UINT64>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_UINT64>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::UInt64Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteUInt64NoTagToArray(wire_value,
                                                                            &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_UINT32>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_UINT32>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::UInt32Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteUInt32NoTagToArray(wire_value,
                                                                            &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_SINT64>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_SINT64>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::SInt64Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteSInt64NoTagToArray(wire_value,
                                                                            &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_SINT32>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_SINT32>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::SInt32Size(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteSInt32NoTagToArray(wire_value,
                                                                            &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_ENUM>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_ENUM>
{
    unsigned byte_size(const WireType& wire_value)
    {
        return google::protobuf::internal::WireFormatLite::EnumSize(wire_value);
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteEnumNoTagToArray(wire_value, &(*bytes)[0]);
    }
    bool is_varint() { return true; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_DOUBLE>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_DOUBLE>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kDoubleSize;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteDoubleNoTagToArray(wire_value,
                                                                            &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_FLOAT>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_FLOAT>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kFloatSize;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteFloatNoTagToArray(wire_value,
                                                                           &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_BOOL>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_BOOL>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kBoolSize;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteBoolNoTagToArray(wire_value, &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_FIXED64>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_FIXED64>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kFixed64Size;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteFixed64NoTagToArray(wire_value,
                                                                             &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_FIXED32>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_FIXED32>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kFixed32Size;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteFixed32NoTagToArray(wire_value,
                                                                             &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_SFIXED64>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_SFIXED64>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kSFixed64Size;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteSFixed64NoTagToArray(wire_value,
                                                                              &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType>
struct PrimitiveTypeHelper<WireType, google::protobuf::FieldDescriptor::TYPE_SFIXED32>
    : public PrimitiveTypeHelperBase<WireType,
                                     google::protobuf::internal::WireFormatLite::TYPE_SFIXED32>
{
    unsigned byte_size(const WireType& /*wire_value*/)
    {
        return google::protobuf::internal::WireFormatLite::kSFixed32Size;
    }
    void encode(WireType wire_value, std::vector<google::protobuf::uint8>* bytes)
    {
        google::protobuf::internal::WireFormatLite::WriteSFixed32NoTagToArray(wire_value,
                                                                              &(*bytes)[0]);
    }
    bool is_varint() { return false; }
};

template <typename WireType, google::protobuf::FieldDescriptor::Type DeclaredType,
          typename FieldType = WireType>
class PrimitiveTypeFieldCodec : public TypedFieldCodec<WireType, FieldType>
{
  private:
    unsigned presence_bit_size() { return this->use_required() ? 0 : 1; }

    unsigned min_size() override
    {
        // if required, minimum size is 1-byte (for varint) or full size (for non-varint)
        if (this->use_required())
        {
            if (helper_.is_varint())
                return BITS_IN_BYTE;
            else
                return BITS_IN_BYTE * helper_.byte_size(WireType());
        }
        // if not required, presence bit
        else
        {
            return presence_bit_size();
        }
    }

    unsigned max_size() override
    {
        // Int32 and Int64 use more space for large negative numbers
        return std::max<unsigned>(size(std::numeric_limits<WireType>::min()),
                                  size(std::numeric_limits<WireType>::max()));
    }

    unsigned size() override { return min_size(); }

    unsigned size(const WireType& wire_value) override
    {
        unsigned data_bytes = helper_.byte_size(wire_value);
        unsigned size = presence_bit_size() + BITS_IN_BYTE * data_bytes;
        return size;
    }

    Bitset encode() override
    {
        // presence bit, not set
        return Bitset(min_size(), 0);
    }

    Bitset encode(const WireType& wire_value) override
    {
        std::vector<google::protobuf::uint8> bytes(size(wire_value) / BITS_IN_BYTE, 0);

        helper_.encode(wire_value, &bytes);

        Bitset data_bits;
        data_bits.from_byte_stream(bytes.begin(), bytes.end());
        if (!this->use_required())
        {
            data_bits.resize(data_bits.size() + presence_bit_size());
            data_bits <<= 1;
            data_bits.set(0, true); // presence bit
        }
        return data_bits;
    }

    WireType decode(Bitset* bits) override
    {
        if (!this->use_required())
        {
            dccl::uint64 uint_value = (bits->template to<dccl::uint64>)();
            if (!uint_value)
                throw NullValueException();
            bits->resize(0);

            if (helper_.is_varint())
                bits->get_more_bits(BITS_IN_BYTE);
            else
                bits->get_more_bits(BITS_IN_BYTE * helper_.byte_size(WireType()));
        }

        if (helper_.is_varint())
        {
            // most significant bit indicates if more bytes are needed
            while (bits->test(bits->size() - 1)) bits->get_more_bits(BITS_IN_BYTE);
        }

        std::string bytes = bits->to_byte_string();
        google::protobuf::io::CodedInputStream input_stream(
            reinterpret_cast<const google::protobuf::uint8*>(bytes.data()), bytes.size());

        return helper_.decode(&input_stream);
    }

  private:
    PrimitiveTypeHelper<WireType, DeclaredType> helper_;
};

class EnumFieldCodec
    : public PrimitiveTypeFieldCodec<int, google::protobuf::FieldDescriptor::TYPE_ENUM,
                                     const google::protobuf::EnumValueDescriptor*>
{
  public:
    int pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value) override;
    const google::protobuf::EnumValueDescriptor* post_decode(const int& wire_value) override;
};

} // namespace native_protobuf
} // namespace dccl

#endif
