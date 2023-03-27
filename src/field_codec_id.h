// Copyright 2009-2017:
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
#include "field_codec_fixed.h"

namespace dccl
{
/// \brief Provides the default 1 byte or 2 byte DCCL ID codec
class DefaultIdentifierCodec : public TypedFieldCodec<uint32>
{
  protected:
    Bitset encode() override;
    Bitset encode(const uint32& wire_value) override;
    uint32 decode(Bitset* bits) override;
    unsigned size() override;
    unsigned size(const uint32& wire_value) override;
    unsigned max_size() override;
    unsigned min_size() override;
    void validate() override {}

  private:
    unsigned this_size(const uint32& wire_value);
    // maximum id we can fit in short or long header (MSB reserved to indicate
    // short or long header)
    enum
    {
        ONE_BYTE_MAX_ID = (1 << 7) - 1,
        TWO_BYTE_MAX_ID = (1 << 15) - 1
    };

    enum
    {
        SHORT_FORM_ID_BYTES = 1,
        LONG_FORM_ID_BYTES = 2
    };
};
} // namespace dccl
