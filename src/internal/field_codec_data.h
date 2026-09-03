// Copyright 2023:
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
#ifndef DCCLFIELDCODECDATAH
#define DCCLFIELDCODECDATAH

#include "../dynamic_conditions.h"

#include "field_codec_message_stack.h"

#include <typeindex>

namespace google
{
namespace protobuf
{
class Message;
class Descriptor;
} // namespace protobuf
} // namespace google

namespace dccl
{
namespace internal
{
// Data shared amongst all the FieldCodecs for a single message
struct CodecData
{
    MessagePart part_{dccl::UNKNOWN};
    bool strict_{false};
    const google::protobuf::Message* root_message_{nullptr};
    const google::protobuf::Descriptor* root_descriptor_{nullptr};
    MessageStackData message_data_;
    DynamicConditions dynamic_conditions_;

    // Pointer to the root Bitset for the current encode or decode operation.
    //
    // During encoding: updated by field_encode() to point to the accumulation Bitset for each
    // field. When any_encode() is called for a field, root_bits_ holds all bits encoded by
    // previous sibling fields in the same message.
    //
    // During decoding: points to decoded_bits_, which accumulates bits for fields already
    // decoded in the current message. When post_decode() is called for a field, root_bits_
    // holds all bits decoded by previous sibling fields.
    //
    // Returns nullptr outside of an encode/decode context (e.g., during validate/info).
    Bitset* root_bits_{nullptr};
    // Snapshot of the part (head or body) taken before any of it is decoded. Codecs are free
    // to modify the Bitset they decode from, so the bits a field consumed are recovered from
    // here rather than read back out of that Bitset.
    Bitset stream_bits_;
    Bitset decoded_bits_; // accumulates decoded bits during base_decode()

    template <typename FieldCodecType> void set_codec_specific_data(std::shared_ptr<dccl::any> data)
    {
        codec_specific_[std::type_index(typeid(FieldCodecType))] = data;
    }

    template <typename FieldCodecType> std::shared_ptr<dccl::any> codec_specific_data()
    {
        return codec_specific_.at(std::type_index(typeid(FieldCodecType)));
    }

    template <typename FieldCodecType> bool has_codec_specific_data()
    {
        return codec_specific_.count(std::type_index(typeid(FieldCodecType)));
    }

  private:
    std::map<std::type_index, std::shared_ptr<dccl::any>> codec_specific_;
};
} // namespace internal
} // namespace dccl

#endif
