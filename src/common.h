// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
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

#include <iostream>
#include <cmath>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "dccl/bitset.h"


namespace dccl
{
    inline unsigned floor_bits2bytes(unsigned bits)
    { return bits >> 3; }
    // more efficient way to do ceil(total_bits / 8)
    // to get the number of bytes rounded up.
    inline unsigned ceil_bits2bytes(unsigned bits)
    {
        enum { BYTE_MASK = 7 }; // 00000111
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
    
    inline std::ostream& operator<<(std::ostream& out,
                                    const google::protobuf::Message& msg)
    {
        return (out << "[["
                << msg.GetDescriptor()->name()
                << "]] " << msg.DebugString());
    }


    /// round 'r' to 'dec' number of decimal places
    /// we want no upward bias so
    /// round 5 up if odd next to it, down if even
    /// \param r value to round
    /// \param dec number of places past the decimal to round (e.g. dec=1 rounds to tenths)
    /// \return r rounded

    inline double unbiased_round(double r, double dec)
    {
        double ex = std::pow(10.0, dec);
        double final = std::floor(r * ex);
        double s = (r * ex) - final;

        // remainder less than 0.5 or even number next to it
        if (s < 0.5 || (s==0.5 && !(static_cast<unsigned long>(final)&1)))
            return final / ex;
        else 
            return (final+1) / ex;
    }


    
}
#endif
