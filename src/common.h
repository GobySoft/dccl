// Copyright 2009-2023:
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

#include <bitset>
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

/// decomposes float/double to a two-integer representation: a significant (aka mantissa)
/// and exponent. The functionality is similar to std::frexp except the significant is
/// represented as a integer type instead of a floating point representation.
/// Conceptually: val = <return value> * 2 ^ exponent
/// 
/// \param val value to decompose
/// \param exponent that allows the significant to be expressed without a decimal point
/// \return the significand as an integer
/// 
/// If val is 0, exponent is 0, and 0 is returned
/// If val is +Inf, exponent is unspecified, and most positive integer is returned
/// If val is -Inf, exponent is unspecified, and most negative integer is returned
/// If val is NaN, exponent is unspecified, and second most negative integer is returned
inline int32_t decompose_float_format(float val, int16_t& exponent) {
    uint32_t &val_bits = reinterpret_cast<uint32_t&>(val);

    constexpr auto num_exp_bits = 8;
    constexpr auto num_frac_bits = 23;
    constexpr auto exponent_bias = 127;

    constexpr auto sign_bit_mask = static_cast<uint32_t>(0b1) << (num_exp_bits + num_frac_bits);
    constexpr auto exp_bits = (static_cast<uint32_t>(1u)<<num_exp_bits) - 1;
    constexpr auto exp_bit_mask = exp_bits << num_frac_bits;
    constexpr auto frac_bit_mask = (static_cast<uint32_t>(1u) << num_frac_bits) - 1;

    uint32_t signed_bit = val_bits & sign_bit_mask;
    uint16_t val_exp_bits = static_cast<uint16_t>((val_bits & exp_bit_mask) >> num_frac_bits);
    uint32_t frac_bits = val_bits & frac_bit_mask;
    if (val_exp_bits == exp_bits) {
        if (frac_bits == 0) {
            // val is an infinity
            if (signed_bit > 0) {
                return std::numeric_limits<int32_t>::lowest();
            } else {
                return std::numeric_limits<int32_t>::max();
            }
        } else {
            // val is a NaN
            return std::numeric_limits<int32_t>::lowest() + 1;
        }
    }

    if (val_exp_bits == 0) {
        if (frac_bits == 0) {
            exponent = 0;
            // val is zero
            return 0;
        } else {
            // val is a subnormal number
            exponent = -exponent_bias + 1 - num_frac_bits; // shift so that the fraction can be interpreted as full integer
            if (signed_bit > 0) {
                return -static_cast<int32_t>(frac_bits);
            } else {
                return static_cast<int32_t>(frac_bits);
            }
        }
    }

    // val is a normal number
    constexpr auto implicit_bit_mask = static_cast<uint32_t>(1u) << num_frac_bits;
    exponent = static_cast<uint32_t>(val_exp_bits) - exponent_bias - num_frac_bits; // shift so that the fraction can be interpreted as full integer
    if (signed_bit > 0) {
        return -static_cast<int32_t>(frac_bits | implicit_bit_mask);
    } else {
        return static_cast<int32_t>(frac_bits | implicit_bit_mask);
    }
}

inline int64_t decompose_float_format(double val, int16_t& exponent) {
    uint64_t &val_bits = reinterpret_cast<uint64_t&>(val);

    constexpr auto num_exp_bits = 11;
    constexpr auto num_frac_bits = 52;
    constexpr auto exponent_bias = 1023;

    constexpr auto sign_bit_mask = static_cast<uint64_t>(0b1) << (num_exp_bits + num_frac_bits);
    constexpr auto exp_bits = (static_cast<uint64_t>(1u)<<num_exp_bits) - 1;
    constexpr auto exp_bit_mask = exp_bits << num_frac_bits;
    constexpr auto frac_bit_mask = (static_cast<uint64_t>(1u) << num_frac_bits) - 1;

    uint64_t signed_bit = val_bits & sign_bit_mask;
    uint16_t val_exp_bits = static_cast<uint16_t>((val_bits & exp_bit_mask) >> num_frac_bits);
    uint64_t frac_bits = val_bits & frac_bit_mask;
    if (val_exp_bits == exp_bits) {
        if (frac_bits == 0) {
            // val is an infinity
            if (signed_bit > 0) {
                return std::numeric_limits<int64_t>::lowest();
            } else {
                return std::numeric_limits<int64_t>::max();
            }
        } else {
            // val is a NaN
            return std::numeric_limits<int64_t>::lowest() + 1;
        }
    }

    if (val_exp_bits == 0) {
        if (frac_bits == 0) {
            exponent = 0;
            // val is zero
            return 0;
        } else {
            // val is a subnormal number
            exponent = -exponent_bias + 1 - num_frac_bits; // shift so that the fraction can be interpreted as full integer
            if (signed_bit > 0) {
                return -static_cast<int64_t>(frac_bits);
            } else {
                return static_cast<int64_t>(frac_bits);
            }
        }
    }

    // val is a normal number
    constexpr auto implicit_bit_mask = static_cast<uint64_t>(1u) << num_frac_bits;
    exponent = static_cast<uint64_t>(val_exp_bits) - exponent_bias - num_frac_bits; // shift so that the fraction can be interpreted as full integer
    if (signed_bit > 0) {
        return -static_cast<int64_t>(frac_bits | implicit_bit_mask);
    } else {
        return static_cast<int64_t>(frac_bits | implicit_bit_mask);
    }
}

template<std::size_t N>
inline void increment(std::bitset<N> &bits) {
    for (auto i = 0ul; i < N; ++i) {
        if ((bits[i] = !bits[i]) == true) {
            break;
        }
    }
}

template<std::size_t N>
inline bool is_negative(const std::bitset<N> &bits) {
    return bits[N-1];
}

template<std::size_t N>
inline void negate(std::bitset<N> &bits) {
    bits.flip();
    increment(bits);
}

template<std::size_t N>
inline std::bitset<N> negated(const std::bitset<N> &bits) {
    auto ret_val = bits;
    negate(ret_val);
    return ret_val;
}

template<std::size_t N>
inline void add_to(std::bitset<N> &a, const std::bitset<N> &b) {
    bool carry_bit = false;
    for (auto i = 0ul; i < N; ++i) {
        auto sum = carry_bit + a[i] + b[i];
        a[i] = sum % 2;
        carry_bit = sum > 1;
    }
}

template<std::size_t N>
inline std::bitset<N> add(const std::bitset<N> &a, const std::bitset<N> &b) {
    auto ret_val = a;
    add_to(ret_val, b);
    return ret_val;
}

template<std::size_t N>
inline std::bitset<N> subtract(const std::bitset<N> &a, const std::bitset<N> &b) {
    auto ret_val = negated(b);
    add_to(ret_val, a);
    return ret_val;
}

template<std::size_t N>
inline bool unsigned_geq(const std::bitset<N> &a, const std::bitset<N> &b) {
    for (auto k = 0; k < N; ++k) {
        const auto i = N - 1 - k;
        if (a[i] > b[i]) {
            return true;
        } else if (b[i] > a[i]) {
            return false;
        }
    }
    return true;
}

// Reference: see Long division at https://en.wikipedia.org/wiki/Division_algorithm
template<std::size_t N>
inline std::bitset<N> unsigned_divide(const std::bitset<N> &n, const std::bitset<N> &d) {
    auto q = std::bitset<N>{0};
    auto r = std::bitset<N>{0};
    for (auto k = 0; k < N; ++k) {
        const auto i = N - 1 - k;
        r <<= 1;
        r[0] = n[i];
        if (unsigned_geq(r, d)) {
            r = subtract(r, d);
            q[i] = true;
        }
    }

    // Remainder for rounding
    r <<= 1;
    if (unsigned_geq(r, d)) {
        increment(q);
    }

    return q;
}

} // namespace dccl
#endif
