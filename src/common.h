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

    template<typename Float>
        Float round(Float d)
    { return std::floor(d + 0.5); }
    
    /// round 'value' to 'precision' number of decimal places
    /// \param r value to round
    /// \param dec number of places past the decimal to round (e.g. dec=1 rounds to tenths)
    /// \return r rounded
    template<typename Float>
        typename boost::enable_if<boost::is_floating_point<Float>, Float>::type round(Float value, int precision)
    {
        Float scaling = std::pow(10.0, precision);
        return round(value*scaling)/scaling;        
    }
    
    // C++98 has no long long overload for abs
    template<typename Int>
      Int abs(Int i) { return (i < 0) ? -i : i; }

    /// round 'value' to 'precision' number of decimal places
    /// \param r value to round
    /// \param dec number of places past the decimal to round (e.g. dec=1 rounds to tenths)
    /// \return r rounded
    template<typename Int>
        typename boost::enable_if<boost::is_integral<Int>, Int>::type round(Int value, int precision)
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
            if(remainder >= scaling/2)
                value += scaling;

            return value;
        }
    }
    
    
}
#endif
