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



#ifndef DCCLConstants20091211H
#define DCCLConstants20091211H

#include "dccl/bitset.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <iostream>

namespace dccl
{
    // more efficient way to do ceil(total_bits / 8)
    // to get the number of bytes rounded up.
    enum { BYTE_MASK = 7 }; // 00000111
    inline unsigned floor_bits2bytes(unsigned bits)
    { return bits >> 3; }
    inline unsigned ceil_bits2bytes(unsigned bits)
    {
        return (bits& BYTE_MASK) ?
            floor_bits2bytes(bits) + 1 :
            floor_bits2bytes(bits);
    }      


    // use the Google Protobuf types as they handle system quirks already
    /// an unsigned 32 bit integer
    typedef google::protobuf::uint32 uint32;
    /// a signed 32 bit integer
    typedef google::protobuf::int32 int32;
    /// an unsigned 64 bit integer
    typedef google::protobuf::uint64 uint64;
    /// a signed 64 bit integer
    typedef google::protobuf::int64 int64;

    const unsigned BITS_IN_BYTE = 8;
    // one hex char is a nibble (4 bits), two nibbles per byte
    const unsigned NIBS_IN_BYTE = 2;

    /// special modem id for the broadcast destination - no one is assigned this address. Analogous to 192.168.1.255 on a 192.168.1.0 subnet
    const int BROADCAST_ID = 0;
        
    
    inline std::ostream& operator<<(std::ostream& out,
                                    const google::protobuf::Message& msg)
    {
        return (out << "[["
                << msg.GetDescriptor()->name()
                << "]] " << msg.DebugString());
    }

    
}
#endif
