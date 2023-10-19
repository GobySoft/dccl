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

#ifndef DCCLV4FIELDCODECHASHH
#define DCCLV4FIELDCODECHASHH

#include "field_codec_default.h"

namespace dccl
{
namespace v4
{

class HashCodec : public v4::DefaultNumericFieldCodec<uint32>
{
  public:
    HashCodec() { this->set_force_use_required(); }

  private:
    Bitset encode() override
    {
        return this->v4::DefaultNumericFieldCodec<uint32>::encode(masked_hash());
    }

    uint32 pre_encode(const uint32& field_value) override { return masked_hash(); }
    uint32 post_decode(const uint32& wire_value) override
    {
        uint32 expected = masked_hash();
        uint32 received = wire_value;

        if (expected != received)
        {
            std::stringstream ss;
            ss << "Hash value mismatch. Expected: " << expected << ", received: " << received
               << ". Ensure both sender and receiver are using identical DCCL definitions";

            throw(Exception(ss.str(), this->root_descriptor()));
        }
        return wire_value;
    }

    void validate() override
    {
        FieldCodecBase::require(this->min() == 0, "(dccl.field).min must equal 0");

        uint32 max = static_cast<uint32>(this->max());
        uint32 max_should_be = (1ull << (ceil_log2(this->max()))) - 1;
        FieldCodecBase::require(max == max_should_be,
                                "(dccl.field).max must equal be a power of 2 minus 1 "
                                "(e.g. 255, 4294967295, etc.)");
        FieldCodecBase::require(this->max() <=
                                    static_cast<double>(std::numeric_limits<uint32>::max()),
                                "(dccl.field).max must fit in unsigned 32 bits.");
    }

    uint32 masked_hash()
    {
        std::size_t hash = this->manager().hash(this->root_descriptor());
        uint32 mask = static_cast<uint32>(this->max());
        return mask & hash;
    }
};

} // namespace v4
} // namespace dccl

#endif
