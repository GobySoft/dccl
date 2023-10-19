// Copyright 2009-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Davide Fenucci <davfen@noc.ac.uk>
//   Chris Murphy <cmurphy@aphysci.com>
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
#ifndef DCCLConstants20091211H
#define DCCLConstants20091211H

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <type_traits>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "bitset.h"

namespace dccl
{
inline unsigned floor_bits2bytes(unsigned bits) { return bits >> 3; }
// more efficient way to do ceil(total_bits / 8)
// to get the number of bytes rounded up.
inline unsigned ceil_bits2bytes(unsigned bits)
{
    enum
    {
        BYTE_MASK = 7
    }; // 00000111
    return (bits & BYTE_MASK) ? floor_bits2bytes(bits) + 1 : floor_bits2bytes(bits);
}

// use the Google Protobuf types as they handle system quirks already
/// an unsigned 32 bit integer
typedef google::protobuf::uint32 uint32;
/// a signed 32 bit integer
using int32 = google::protobuf::int32;
/// an unsigned 64 bit integer
using uint64 = google::protobuf::uint64;
/// a signed 64 bit integer
using int64 = google::protobuf::int64;

const unsigned BITS_IN_BYTE = 8;

inline std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& msg)
{
    return (out << "[[" << msg.GetDescriptor()->name() << "]] " << msg.DebugString());
}

template <typename Float> Float round(Float d) { return std::floor(d + 0.5); }

/// round 'value' to 'precision' number of decimal places
/// \param value value to round
/// \param precision number of places past the decimal to round (e.g. dec=1 rounds to tenths)
/// \return rounded value
template <typename Float>
typename std::enable_if<std::is_floating_point<Float>::value, Float>::type round(Float value,
                                                                                 int precision)
{
    Float scaling = std::pow(10.0, precision);
    return round(value * scaling) / scaling;
}

/// approximate 'value' to the nearest quantile defined by 'interval'
/// \param value value to round
/// \param interval number defining the quantization step
/// \return quantized value
template <typename Float>
typename std::enable_if<std::is_floating_point<Float>::value, Float>::type quantize(Float value,
                                                                                    double interval)
{
    if (interval >= 1)
        return round(value / interval) * interval;
    else
    {
        double interval_inv = 1.0 / interval;
        return round(value * interval_inv) / interval_inv;
    }
}

// C++98 has no long long overload for abs
template <typename Int> Int abs(Int i) { return (i < 0) ? -i : i; }

/// round 'value' to 'precision' number of decimal places
/// \param value value to round
/// \param precision number of places past the decimal to round (e.g. dec=1 rounds to tenths)
/// \return rounded value
template <typename Int>
typename std::enable_if<std::is_integral<Int>::value, Int>::type round(Int value, int precision)
{
    if (precision >= 0)
    {
        // doesn't mean anything to round an integer to positive precision
        return value;
    }
    else
    {
        Int scaling = (Int)std::pow(10.0, -precision);
        Int remainder = value % scaling;

        value -= remainder;
        if (remainder >= scaling / 2)
            value += scaling;

        return value;
    }
}

/// approximate 'value' to the nearest quantile defined by 'interval'
/// \param value value to round
/// \param interval number defining the quantization step
/// \return quantized value
template <typename Int>
typename std::enable_if<std::is_integral<Int>::value, Int>::type quantize(Int value,
                                                                          double interval)
{
    if ((interval - static_cast<uint64_t>(interval)) >= std::numeric_limits<double>::epsilon())
    {
        // doesn't mean anything to quantize an integer with a fractional interval
        return value;
    }

    Int remainder = value % static_cast<Int>(interval);
    value -= remainder;
    if (remainder >= interval / 2)
        value += interval;
    return value;
}

/// hash combine - from boost::hash_combine
template <class T> inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace dccl
#endif
