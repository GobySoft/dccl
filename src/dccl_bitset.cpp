// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.


#include "dccl_bitset.h"
#include "dccl/dccl.h"

using namespace dccl::logger;

dccl::Bitset dccl::Bitset::relinquish_bits(size_type num_bits,
                                                           bool final_child)
{
//    dlog.is(DEBUG2) && dlog  << group(Codec::dlog_decode_group()) << "Asked to relinquish " << num_bits << " from " << this << ": " << *this << std::endl;

    if(final_child || this->size() < num_bits)
    {
        size_type num_parent_bits = (final_child) ? num_bits : num_bits - this->size();
//        dlog.is(DEBUG2) && dlog  << group(Codec::dlog_decode_group()) << "Need to get " << num_parent_bits << " from parent" << std::endl;

        if(parent_)
        {
            Bitset parent_bits = parent_->relinquish_bits(num_parent_bits, false);
            
//            dlog.is(DEBUG2) && dlog  << group(Codec::dlog_decode_group()) << "parent_bits: " << parent_bits << std::endl;
            
            append(parent_bits);
        }
    }

    Bitset out;
    if(!final_child)
    {
        for(size_type i = 0; i < num_bits; ++i)
        {
            if(this->empty())
                throw(dccl::Exception("Cannot get_more_bits - no more bits to get!"));
            
            out.push_back(this->front());
            this->pop_front();
        }
    }
    return out;
}

