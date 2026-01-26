// Copyright 2009-2025:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Cadmus To <cadmus.to+dev@gmail.com>
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
#ifndef DCCLNumeric20260127H
#define DCCLNumeric20260127H

#include <bitset>
#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>

#include "common.h"

namespace dccl
{

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
int32_t decompose_float_format(float val, int16_t& exponent);
int64_t decompose_float_format(double val, int16_t& exponent);

/// composes float/double from a two-integer representation; the reverse process of
/// decompose_float_format (also accounting for the special cases).
float compose_float_format(int32_t significand, int16_t exponent);
double compose_float_format(int64_t significand, int16_t exponent);

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
inline std::bitset<N> sum(const std::bitset<N> &a, const std::bitset<N> &b) {
    auto ret_val = a;
    add_to(ret_val, b);
    return ret_val;
}

template<std::size_t N>
inline std::bitset<N> difference(const std::bitset<N> &a, const std::bitset<N> &b) {
    auto ret_val = negated(b);
    add_to(ret_val, a);
    return ret_val;
}

template<std::size_t N>
inline bool unsigned_geq(const std::bitset<N> &a, const std::bitset<N> &b) {
    for (auto k = 0ul; k < N; ++k) {
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
inline std::bitset<N> unsigned_quotient(const std::bitset<N> &n, const std::bitset<N> &d) {
    auto q = std::bitset<N>{0};
    auto r = std::bitset<N>{0};
    for (auto k = 0ul; k < N; ++k) {
        const auto i = N - 1 - k;
        r <<= 1;
        r[0] = n[i];
        if (unsigned_geq(r, d)) {
            r = difference(r, d);
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

// Reference: see Other notations at https://en.wikipedia.org/wiki/Multiplication_algorithm
template<std::size_t N>
inline std::bitset<2*N> unsigned_product(const std::bitset<N> &a, const std::bitset<N> &b) {
    auto p = std::bitset<2*N>{0};
    for (auto b_i = 0ul; b_i < N; ++b_i) {
        bool carry = false;
        for (auto a_i = 0ul; a_i < N; ++a_i) {
            auto sum = p[a_i + b_i] + carry + (a[a_i] * b[b_i]);
            carry = static_cast<bool>(sum >> 1);
            p[a_i + b_i] = static_cast<bool>(sum % 2);
        }
        p[b_i + N] = carry;
    }

    return p;
}

template <std::size_t N>
inline void rounding_shift_right(std::bitset<N> &bits, uint16_t num_pos) {
    // Inspect most significant bit for rounding
    bool need_to_increment = bits[num_pos-1];
    bits >>= num_pos;
    if (need_to_increment) {
        increment(bits);
    }
}

// Drops least significant bits until it is represented with the given
// number of significant bits. The most significant dropped bit is used
// to round the result.
// Returns the number of bits dropped.
template <std::size_t N>
inline int16_t drop_to_sig_fig(std::bitset<N> &bits, uint16_t target_sig_fig) {
    if (target_sig_fig == 0) {
        bits = std::bitset<N>{0};
    }

    // Find most significant bit
    auto curr_sig_fig = static_cast<int16_t>(N);
    while (curr_sig_fig > target_sig_fig) {
        if (bits[curr_sig_fig-1]) {
            break;
        } else {
            --curr_sig_fig;
        }
    }

    auto num_bits_to_drop = curr_sig_fig - target_sig_fig;
    if (num_bits_to_drop > 0) {
        rounding_shift_right(bits, num_bits_to_drop);
    }
    return num_bits_to_drop;
}

template <typename T, std::size_t N>
inline T fill_unsigned(const std::bitset<N> &bits) {
    auto ret_val = T{};

    constexpr auto num_type_bits = sizeof(T)*8;
    constexpr auto num_iter = std::min(N, num_type_bits);
    for (auto k = 0ul; k < num_iter; ++k) {
        const auto i = num_iter - 1 - k;
        ret_val <<= 1;
        ret_val += bits[i];
    }

    return ret_val;
}

template <typename Integral, std::enable_if_t<std::is_integral<Integral>::value, bool> = true>
uint64 encode(Integral value, double min, double res) {
    Integral wire_value = dccl::quantize(value, res);

    // calculate the encoded value: remove the minimum, scale for the resolution, cast to int.
    wire_value -= quantize(static_cast<Integral>(min), res);
    if (res >= 1)
        wire_value /= res;
    else
        wire_value *= (1.0 / res);
    return static_cast<uint64>(round(wire_value, 0));
}

template <typename Float, std::enable_if_t<std::is_floating_point<Float>::value, bool> = true>
uint64 encode(Float value, double min, double res) {
    // Float cannot compete with the level of detail capable in (u)int32 types
    // so we need to be careful with the computation. Here we perform computations
    // on the significand/mantissa and exponent values separately for as long as we
    // can, then only convert to a the Float right before rounding.

    int16_t val_exp, res_exp, min_exp;
    auto val_sig_raw = decompose_float_format(value, val_exp);
    auto res_sig_raw = decompose_float_format(static_cast<Float>(res), res_exp);
    auto min_sig_raw = decompose_float_format(static_cast<Float>(min), min_exp);
    // Intentionally reduce precision here. If we can past the float tests with our hands cuffed,
    // then we can be pretty confident when working with doubles too. We should not need the double
    // representation for this to be correct. Also "something something optimisation".

    using sig_t = decltype(val_sig_raw);
    using unsigned_sig_t = typename std::make_unsigned<sig_t>::type;
    constexpr auto num_wider_bits = 2*sizeof(sig_t)*8;
    using wider_t = std::bitset<num_wider_bits>;
    const auto val_sign = std::signbit(val_sig_raw);
    const auto res_sign = std::signbit(res_sig_raw);
    const auto min_sign = std::signbit(min_sig_raw);

    // Reexpress the values in a bitset of positive values
    assert(!res_sign);
    auto val_pos_sig = wider_t{static_cast<unsigned_sig_t>(std::abs(val_sig_raw))};
    auto res_pos_sig = wider_t{static_cast<unsigned_sig_t>(res_sig_raw)};
    auto min_pos_sig = wider_t{static_cast<unsigned_sig_t>(std::abs(min_sig_raw))};

    // reexpress value, minimum, and resolution significands with minimum common exponent
    auto common_exp = std::min({val_exp, min_exp, res_exp});

    auto val_diff = val_exp - common_exp;
    val_pos_sig <<= val_diff;
    val_exp -= val_diff;

    auto min_diff = min_exp - common_exp;
    min_pos_sig <<= min_diff;
    min_exp -= min_diff;

    auto res_diff = res_exp - common_exp;
    res_pos_sig <<= res_diff;
    res_exp -= res_diff;

    // Do the division
    auto quant_val_sig = unsigned_quotient(val_pos_sig, res_pos_sig);
    auto quant_val_exp = val_exp - res_exp;
    auto quant_min_sig = unsigned_quotient(min_pos_sig, res_pos_sig);
    auto quant_min_exp = min_exp - res_exp;

    // Now we get them to exponent zero, rounding if necessary
    if (quant_val_exp < 0) {
        rounding_shift_right(quant_val_sig, static_cast<uint16_t>(-quant_val_exp));
    } else {
        quant_val_sig <<= quant_val_exp;
    }
    quant_val_exp = 0;
    if (quant_min_exp < 0) {
        rounding_shift_right(quant_min_sig, static_cast<uint16_t>(-quant_min_exp));
    } else {
        quant_min_sig <<= quant_min_exp;
    }
    quant_min_exp = 0;

    // Apply signs
    if (val_sign) {
        negate(quant_val_sig);
    }
    if (min_sign) {
        negate(quant_min_sig);
    }

    const auto value_enc_bits = difference(quant_val_sig, quant_min_sig);
    // Encoding expected to be positive
    assert(!is_negative(value_enc_bits));

    const auto value_enc = fill_unsigned<unsigned_sig_t>(value_enc_bits);

    return static_cast<uint64>(value_enc);
}

template <typename Integral, std::enable_if_t<std::is_integral<Integral>::value, bool> = true>
Integral decode(uint64 value_enc, double min, double res) {
    auto wire_value = static_cast<Integral>(value_enc);
    if (res >= 1)
        wire_value *= res;
    else
        wire_value /= (1.0 / res);

    // round values again to properly handle cases where double precision
    // leads to slightly off values (e.g. 2.099999999 instead of 2.1)
    wire_value =
        quantize(wire_value + quantize(static_cast<Integral>(min), res), res);
    return wire_value;
}

template <typename Float, std::enable_if_t<std::is_floating_point<Float>::value, bool> = true>
Float decode(uint64 value_enc, double min, double res) {
    int16_t res_exp, min_exp;
    auto res_sig_raw = decompose_float_format(static_cast<Float>(res), res_exp);
    auto min_sig_raw = decompose_float_format(static_cast<Float>(min), min_exp);

    using sig_t = decltype(res_sig_raw);
    using unsigned_sig_t = typename std::make_unsigned<sig_t>::type;
    constexpr auto num_narrow_bits = sizeof(sig_t)*8;
    constexpr auto num_wider_bits = 2*num_narrow_bits;
    using wider_t = std::bitset<num_wider_bits>;

    const auto res_sign = std::signbit(res_sig_raw);
    const auto min_sign = std::signbit(min_sig_raw);

    // Reexpress the values in a bitset to express negative numbers
    assert(!res_sign);
    auto res_pos_sig = wider_t{static_cast<unsigned_sig_t>(res_sig_raw)};
    auto min_pos_sig = wider_t{static_cast<unsigned_sig_t>(std::abs(min_sig_raw))};
    const auto val_enc_sig = wider_t{value_enc};
    const auto val_enc_exp = 0;

    // Reexpress min and res to the lowest common exponent
    const auto exp_diff = min_exp - res_exp;
    if (exp_diff >= 0) {
        min_pos_sig <<= exp_diff;
        min_exp -= exp_diff;
    } else {
        res_pos_sig <<= -exp_diff;
        res_exp -= -exp_diff;
    }

    auto quant_min_sig = unsigned_quotient(min_pos_sig, res_pos_sig);
    auto quant_min_exp = min_exp - res_exp;

    if (quant_min_exp < 0) {
        rounding_shift_right(quant_min_sig, static_cast<uint16_t>(-quant_min_exp));
    } else {
        quant_min_sig <<= quant_min_exp;
    }
    quant_min_exp = 0;

    // Apply signs
    if (min_sign) {
        negate(quant_min_sig);
    }

    auto sum_pos_bits = sum(val_enc_sig, quant_min_sig);
    const auto sum_sign = is_negative(sum_pos_bits);
    if (sum_sign) {
        negate(sum_pos_bits);
    }
    const auto sum_exp = 0;

    const auto sum_pos_raw_unsigned = fill_unsigned<unsigned_sig_t>(sum_pos_bits);

    auto value = sum_pos_raw_unsigned * res;
    if (sum_sign) {
        value = -value;
    }

    // round values again to properly handle cases where double precision
    // leads to slightly off values (e.g. 2.099999999 instead of 2.1)
    value = quantize(value , res);

    return static_cast<Float>(value);
}

} // namespace dccl
#endif
