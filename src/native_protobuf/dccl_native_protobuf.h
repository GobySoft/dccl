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

#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/wire_format_lite_inl.h>

#include "dccl/field_codec_fixed.h"
#include "dccl/field_codec_typed.h"

namespace dccl
{
/// Implements the default Google Protocol Buffers encoder for a variety of numeric types
namespace native_protobuf
{

// https://developers.google.com/protocol-buffers/docs/encoding
template<typename WireType>
class PrimitiveTypeFieldCodec : public TypedFieldCodec<WireType>
{
private:
    unsigned presence_bit_size()
        {
            return this->use_required() ? 0 : 1;
        }
    
    unsigned min_size()
        {
            // minimum size is 1-byte if required, or presence bit if not
            return this->use_required() ? BITS_IN_BYTE : presence_bit_size();
        }

    unsigned max_size()
        {
            // Int32 and Int64 use more space for large negative numbers
            return std::max<unsigned>(size(std::numeric_limits<WireType>::min()),
                                      size(std::numeric_limits<WireType>::max()));
        }
    
    unsigned size() 
        {
            return min_size();
        }
    
    unsigned size(const WireType& wire_value)
        {
            unsigned data_bytes = 0;
            using google::protobuf::FieldDescriptor;
            switch(this->FieldCodecBase::field_type())
            {
                // variable size
                case FieldDescriptor::TYPE_INT64:
                    data_bytes = google::protobuf::internal::WireFormatLite::Int64Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_INT32:
                    data_bytes = google::protobuf::internal::WireFormatLite::Int32Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_UINT64:
                    data_bytes = google::protobuf::internal::WireFormatLite::UInt64Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_UINT32:
                    data_bytes = google::protobuf::internal::WireFormatLite::UInt32Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_SINT64:
                    data_bytes = google::protobuf::internal::WireFormatLite::SInt64Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_SINT32:
                    data_bytes = google::protobuf::internal::WireFormatLite::SInt32Size(wire_value);
                    break;
                case FieldDescriptor::TYPE_ENUM:
                    data_bytes = google::protobuf::internal::WireFormatLite::EnumSize(wire_value);
                    break;

                // fixed size
                case FieldDescriptor::TYPE_DOUBLE:
                    data_bytes = google::protobuf::internal::WireFormatLite::kDoubleSize;
                    break;
                case FieldDescriptor::TYPE_FLOAT:
                    data_bytes = google::protobuf::internal::WireFormatLite::kFloatSize;
                    break;
                case FieldDescriptor::TYPE_FIXED64:
                    data_bytes = google::protobuf::internal::WireFormatLite::kFixed64Size;
                    break;
                case FieldDescriptor::TYPE_FIXED32:
                    data_bytes = google::protobuf::internal::WireFormatLite::kFixed32Size;
                    break;
                case FieldDescriptor::TYPE_BOOL:
                    data_bytes = google::protobuf::internal::WireFormatLite::kBoolSize;
                    break;
                case FieldDescriptor::TYPE_SFIXED32:
                    data_bytes = google::protobuf::internal::WireFormatLite::kSFixed32Size;
                    break;
                case FieldDescriptor::TYPE_SFIXED64:
                    data_bytes = google::protobuf::internal::WireFormatLite::kSFixed64Size;
                    break;

                default:
                    throw_unsupported_type();
                    break;
            }
            return presence_bit_size() + BITS_IN_BYTE*data_bytes;
        }

    void throw_unsupported_type()
        {
            throw(dccl::Exception(std::string("Type ") + google::protobuf::FieldDescriptor::TypeName(this->FieldCodecBase::field_type())  + " is not supported by PrimitiveTypeFieldCodec"));
        }
    
    Bitset encode()
        {
            // presence bit, not set
            return Bitset(min_size(), 0);
        }
    
    Bitset encode(const WireType& wire_value)
        {
            std::vector<google::protobuf::uint8> bytes(size(wire_value) / BITS_IN_BYTE, 0);
            
            using google::protobuf::FieldDescriptor;
            switch(this->FieldCodecBase::field_type())
            {
                // variable size
                case FieldDescriptor::TYPE_INT64:
                    google::protobuf::internal::WireFormatLite::WriteInt64NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_INT32:
                    google::protobuf::internal::WireFormatLite::WriteInt32NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_UINT64:
                    google::protobuf::internal::WireFormatLite::WriteUInt64NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_UINT32:
                    google::protobuf::internal::WireFormatLite::WriteUInt32NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_SINT64:
                    google::protobuf::internal::WireFormatLite::WriteSInt64NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_SINT32:
                    google::protobuf::internal::WireFormatLite::WriteSInt32NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_ENUM:
                    google::protobuf::internal::WireFormatLite::WriteEnumNoTagToArray(wire_value, &bytes[0]);
                    break;

                // fixed size
                case FieldDescriptor::TYPE_DOUBLE:
                    google::protobuf::internal::WireFormatLite::WriteDoubleNoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_FLOAT:
                    google::protobuf::internal::WireFormatLite::WriteFloatNoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_FIXED64:
                    google::protobuf::internal::WireFormatLite::WriteFixed64NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_FIXED32:
                    google::protobuf::internal::WireFormatLite::WriteFixed32NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_BOOL:
                    google::protobuf::internal::WireFormatLite::WriteBoolNoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_SFIXED32:
                    google::protobuf::internal::WireFormatLite::WriteSFixed32NoTagToArray(wire_value, &bytes[0]);
                    break;
                case FieldDescriptor::TYPE_SFIXED64:
                    google::protobuf::internal::WireFormatLite::WriteSFixed64NoTagToArray(wire_value, &bytes[0]);
                    break;

                default:
                    throw_unsupported_type();
                    break;
            }

            Bitset data_bits;
            data_bits.from_byte_stream(bytes.begin(), bytes.end());
            if(!this->use_required())
                data_bits.prepend(Bitset((presence_bit_size(), 1)));
            
            return data_bits;
        }

    bool is_varint()
        {
            using google::protobuf::FieldDescriptor;
            switch(this->FieldCodecBase::field_type())
            {
                // variable size
                case FieldDescriptor::TYPE_INT64:
                case FieldDescriptor::TYPE_INT32:
                case FieldDescriptor::TYPE_UINT64:
                case FieldDescriptor::TYPE_UINT32:
                case FieldDescriptor::TYPE_SINT64:
                case FieldDescriptor::TYPE_SINT32:
                case FieldDescriptor::TYPE_ENUM:
                    return true;
                    
                // fixed size
                case FieldDescriptor::TYPE_DOUBLE:
                case FieldDescriptor::TYPE_FLOAT:
                case FieldDescriptor::TYPE_FIXED64:
                case FieldDescriptor::TYPE_FIXED32:
                case FieldDescriptor::TYPE_BOOL:
                case FieldDescriptor::TYPE_SFIXED32:
                case FieldDescriptor::TYPE_SFIXED64:
                    return false;
                    
                default:
                    throw_unsupported_type();
                    break;
            }
        }
    

    
    WireType decode(Bitset* bits)
        {
            if(!this->use_required())
            {
                dccl::uint64 uint_value = (bits->template to<dccl::uint64>)();
                if(!uint_value) throw NullValueException();
                bits->resize(0);
                bits->get_more_bits(BITS_IN_BYTE);
            }

            if(is_varint())
            {
                // most significant bit indicates if more bytes are needed
                while(bits->test(bits->size()-1))
                    bits->get_more_bits(BITS_IN_BYTE);
            }

            std::string bytes = bits->to_byte_string();
            google::protobuf::io::CodedInputStream input_stream(reinterpret_cast<const google::protobuf::uint8*>(bytes.data()), bytes.size());

            using google::protobuf::FieldDescriptor;
            using google::protobuf::internal::WireFormatLite;
            WireType wire_value;
            switch(this->FieldCodecBase::field_type())
            {
                // variable size
                case FieldDescriptor::TYPE_INT64:
                {
                    google::protobuf::int64 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int64,
                                                  WireFormatLite::TYPE_INT64>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_INT32:
                {
                    google::protobuf::int32 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int32,
                                                  WireFormatLite::TYPE_INT32>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_UINT64:
                {
                    google::protobuf::uint64 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::uint64,
                                                  WireFormatLite::TYPE_UINT64>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_UINT32:
                {
                    google::protobuf::uint32 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::uint32,
                                                  WireFormatLite::TYPE_UINT32>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_SINT64:
                {
                    google::protobuf::int64 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int64,
                                                  WireFormatLite::TYPE_SINT64>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_SINT32:
                {
                    google::protobuf::int32 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int32,
                                                  WireFormatLite::TYPE_SINT32>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                case FieldDescriptor::TYPE_ENUM:
                {
                    int value;
                    WireFormatLite::ReadPrimitive<int,
                                                  WireFormatLite::TYPE_ENUM>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                // fixed size
                case FieldDescriptor::TYPE_DOUBLE:
                {
                    double value;
                    WireFormatLite::ReadPrimitive<double,
                                                  WireFormatLite::TYPE_DOUBLE>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_FLOAT:
                {
                    float value;
                    WireFormatLite::ReadPrimitive<float,
                                                  WireFormatLite::TYPE_FLOAT>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_FIXED64:
                {
                    google::protobuf::uint64 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::uint64,
                                                  WireFormatLite::TYPE_FIXED64>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_FIXED32:
                {
                    google::protobuf::uint32 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::uint32,
                                                  WireFormatLite::TYPE_FIXED32>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_BOOL:
                {    
                    bool value;
                    WireFormatLite::ReadPrimitive<bool,
                                                  WireFormatLite::TYPE_BOOL>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_SFIXED32:
                {
                    google::protobuf::int32 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int32,
                                                  WireFormatLite::TYPE_SFIXED32>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                
                case FieldDescriptor::TYPE_SFIXED64:
                {
                    google::protobuf::int64 value;
                    WireFormatLite::ReadPrimitive<google::protobuf::int64,
                                                  WireFormatLite::TYPE_SFIXED64>(
                                                      &input_stream, &value);
                    wire_value = value;
                    break;
                }
                default:
                    throw_unsupported_type();
                    break;
            }            
            return wire_value;
        }    

private:
    

};
    
}
}



#endif
