// Copyright 2019-2023:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
#ifndef FIELD_CODEC_PRESENCE_20190722H
#define FIELD_CODEC_PRESENCE_20190722H

#include "../field_codec_typed.h"

namespace dccl
{
namespace v3
{
/// \brief Encodes empty optional fields with a single "presence" bit.
///
/// This class wraps an existing TypedFieldCodec, adding an extra "presence" bit to the LSB for optional fields.
/// The field size is as follows:
/// - Optional
///     - Empty: 1 bit
///     - Non-empty: size of WrappedType + 1
/// - Required
///     - Empty: size of WrappedType
///     - Non-empty: size of WrappedType
template <typename WrappedType>
class PresenceBitCodec
    : public TypedFieldCodec<typename WrappedType::wire_type, typename WrappedType::field_type>
{
  public:
    using Base = TypedFieldCodec<typename WrappedType::wire_type, typename WrappedType::field_type>;

    /// The codec type of the "wrapped" codec
    using wrapped_type = WrappedType;

    /// The wire_type of the "wrapped" codec
    using wire_type = typename Base::wire_type;

    /// The field_type of the "wrapped" codec
    using field_type = typename Base::field_type;

  private:
    /// Instance of the "wrapped" codec
    WrappedType _inner_codec;

  public:
    PresenceBitCodec() { _inner_codec.set_force_use_required(true); }

    // required when wire_type != field_type
    wire_type pre_encode(const field_type& field_value) override
    {
        return _inner_codec.pre_encode(field_value);
    }

    // required when wire_type != field_type
    field_type post_decode(const wire_type& wire_value) override
    {
        return _inner_codec.post_decode(wire_value);
    }

    /// Calls _inner_codec.validate()
    void validate() override { _inner_codec.validate(); }

    /// Encodes an empty field as a single 0 bit
    Bitset encode() override
    {
        return Bitset(1, 0); // presence bit == false
    }

    /// Encodes a non-empty field, adding a 1 bit to the front for optional fields
    Bitset encode(const wire_type& value) override
    {
        Bitset encoded = _inner_codec.encode(value);

        if (!this->use_required())
        {
            encoded.push_front(true);
        }
        return encoded;
    }

    /// Decodes a field, first evaluating the presence bit if necessary
    wire_type decode(Bitset* bits) override
    {
        if (!this->use_required())
        {
            // optional field: bits contains only the presence bit
            bool present = bits->front();
            if (!present)
            {
                throw NullValueException();
            }
            // the single bit was the presence bit; consume it and get the rest
            bits->pop_front();
            bits->get_more_bits(_inner_codec.size());
        }

        return _inner_codec.decode(bits);
    }

    /// Size of an empty field (1 bit)
    unsigned size() override
    {
        // empty field is always 1 bit (only occurs for optional fields)
        return 1;
    }

    /// Size of a non-empty field; gets size from "wrapped" codec, adds 1 for optional fields
    unsigned size(const wire_type& wire_value) override
    {
        int presence_bits = this->use_required() ? 0 : 1;
        return presence_bits + _inner_codec.size(wire_value);
    }

    unsigned max_size() override
    {
        if (this->use_required())
        {
            // def to "wrapped" codec
            return _inner_codec.max_size();
        }
        else
        {
            // optional fields have the extra presence bit
            return _inner_codec.max_size() + 1;
        }
    }

    unsigned min_size() override
    {
        if (this->use_required())
        {
            // defer to "wrapped" codec
            return _inner_codec.min_size();
        }
        else
        {
            // optional fields encode as 1 bit minimum
            return 1;
        }
    }

    void set_manager(FieldCodecManagerLocal* manager) override
    {
        _inner_codec.set_manager(manager);
        FieldCodecBase::set_manager(manager);
    }
};
} // namespace v3
} // namespace dccl

#endif /* FIELD_CODEC_PRESENCE_20190722H */
