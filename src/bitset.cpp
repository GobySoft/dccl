// Copyright 2012-2017:
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
#include "bitset.h"
#include "dccl/codec.h"

using namespace dccl::logger;

dccl::Bitset dccl::Bitset::relinquish_bits(size_type num_bits, bool final_child)
{
    if (final_child || this->size() < num_bits)
    {
        size_type num_parent_bits = (final_child) ? num_bits : num_bits - this->size();
        if (parent_)
        {
            Bitset parent_bits = parent_->relinquish_bits(num_parent_bits, false);
            append(parent_bits);
        }
    }

    Bitset out;
    if (!final_child)
    {
        for (size_type i = 0; i < num_bits; ++i)
        {
            if (this->empty())
                throw(dccl::Exception("Cannot relinquish_bits - no more bits to give up! Check "
                                      "that all field codecs are always producing (encode) and "
                                      "consuming (decode) the exact same number of bits."));

            out.push_back(this->front());
            this->pop_front();
        }
    }
    return out;
}
