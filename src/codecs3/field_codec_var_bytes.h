// Copyright 2018:
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
#ifndef FIELD_CODEC_VAR_BYTES_20181010H
#define FIELD_CODEC_VAR_BYTES_20181010H

#include "dccl/field_codec_typed.h"

namespace dccl
{
    namespace v3
    {
        // Size (in bits) for "required/optional/repeated bytes foo":
        // if optional: [1 bit - has_foo()?][N bits - prefix with length of byte string][string bytes]
        // if required: [N bits - prefix with length of string][string bytes]
        // if repeated: [M bits - prefix with the number of repeated values][same as "required" for value with index 0][same as "required" for index = 1]...[same as required for last index]
        class VarBytesCodec : public dccl::TypedFieldCodec<std::string>
        {
        private:
            dccl::Bitset encode();
            dccl::Bitset encode(const std::string& wire_value);
            std::string decode(dccl::Bitset* bits);
            unsigned size();
            unsigned size(const std::string& wire_value);
            unsigned max_size();
            unsigned min_size();
            void validate();
        
        private:
            unsigned prefix_size()
            { return dccl::ceil_log2(dccl_field_options().max_length()+1); }
            unsigned presence_size()
            { return use_required() ? 0 : 1; }
        
        };
    }
}

#endif
