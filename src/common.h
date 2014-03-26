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
#include <limits>

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/utility/enable_if.hpp>

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

    inline bool are_same(double a, double b)
    {
        return std::fabs(a - b) < std::numeric_limits<double>::epsilon();
    }
    

    /// round 'r' to 'dec' number of decimal places
    /// we want no upward bias so
    /// round 5 up if odd next to it, down if even
    /// So both 1.35 and 1.45 rounded to precision = 1 (tenths) is 1.4
    /// \param r value to round
    /// \param dec number of places past the decimal to round (e.g. dec=1 rounds to tenths)
    /// \return r rounded
    template<typename Float>
        typename boost::enable_if<boost::is_floating_point<Float>, Float>::type unbiased_round(Float value, int precision)
    {
        Float scaling = std::pow(10.0, precision);

        Float intpart = 0;
        Float remainder = std::modf(value * scaling, &intpart); // scale value up and split into int/frac components

        // figure out if we need to add a value for rounding up
        if (remainder > 0.5 || // is greater than 0.5
            (are_same(remainder, 0.5) && ((unsigned)std::abs(intpart) & 1))) // is 0.5 and next place is odd
            intpart += 1;

        // scale back to original
        return intpart / scaling;
        
    }
    
    // C++98 has no long long overload for abs
    template<typename Int>
      Int abs(Int i) { return (i < 0) ? -i : i; }

    template<typename Int>
        typename boost::enable_if<boost::is_integral<Int>, Int>::type unbiased_round(Int value, int precision)
    {
        if(precision >= 0)
        {
            // doesn't mean anything to round an integer to positive precision
            return value;
        }
        else
        {
	  Int scaling = (Int)std::pow(10.0, -precision);
            Int remainder = value % scaling;

            value -= remainder;
            if(remainder > scaling/2 || // is greater than 0.5
               (remainder == scaling / 2 && (dccl::abs<Int>(value)/scaling & 1))) // is 0.5 and next place is odd
            {
                value += scaling;
            }            

            return value;
        }
    }
    
    
}
#endif
