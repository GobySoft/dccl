// Copyright 2026:
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

// Tests the IEEE-754 decompose/compose helpers and the fixed-width bitset
// arithmetic in numeric.h that the arithmetic and v4+ float codecs build on.

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

#include "dccl/logger.h"

#include "../../numeric.h"

namespace
{
template <typename Float, typename Int> void check_special_values()
{
    using FloatLimits = std::numeric_limits<Float>;
    using IntLimits = std::numeric_limits<Int>;

    int16_t exponent = 12345; // sentinel: untouched by the non-finite branches

    assert(dccl::decompose_float_format(FloatLimits::infinity(), exponent) == IntLimits::max());
    assert(dccl::decompose_float_format(-FloatLimits::infinity(), exponent) == IntLimits::lowest());
    assert(dccl::decompose_float_format(FloatLimits::quiet_NaN(), exponent) ==
           IntLimits::lowest() + 1);

    // zero is the one special case that does define the exponent
    assert(dccl::decompose_float_format(static_cast<Float>(0.0), exponent) == 0);
    assert(exponent == 0);

    exponent = 12345;
    assert(dccl::decompose_float_format(static_cast<Float>(-0.0), exponent) == 0);
    assert(exponent == 0);

    assert(dccl::compose_float_format(IntLimits::max(), 0) == FloatLimits::infinity());
    assert(dccl::compose_float_format(IntLimits::lowest(), 0) == -FloatLimits::infinity());
    assert(std::isnan(dccl::compose_float_format(static_cast<Int>(IntLimits::lowest() + 1), 0)));
    assert(dccl::compose_float_format(static_cast<Int>(0), 0) == static_cast<Float>(0.0));
}

// A decompose/compose pair must reproduce the input bit-for-bit.
template <typename Float> void check_round_trip(Float val)
{
    int16_t exponent = 0;
    auto significand = dccl::decompose_float_format(val, exponent);
    Float out = dccl::compose_float_format(significand, exponent);

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "round trip " << val << " -> " << significand
                                                    << " * 2^" << exponent << " -> " << out
                                                    << std::endl;

    assert(out == val);
}

template <typename Float> void check_finite_values()
{
    using FloatLimits = std::numeric_limits<Float>;

    check_round_trip<Float>(1.0);
    check_round_trip<Float>(-1.0);
    check_round_trip<Float>(0.5);
    check_round_trip<Float>(-0.5);
    check_round_trip<Float>(2.0);
    check_round_trip<Float>(1234.5);
    check_round_trip<Float>(-1234.5);

    check_round_trip(FloatLimits::max());
    check_round_trip(FloatLimits::lowest());
    check_round_trip(FloatLimits::min()); // smallest normal
    check_round_trip(-FloatLimits::min());
    check_round_trip(FloatLimits::denorm_min()); // smallest subnormal
    check_round_trip(-FloatLimits::denorm_min());
    check_round_trip(FloatLimits::epsilon());

    // subnormals share one exponent, so the significand alone distinguishes them
    int16_t denorm_exp = 0;
    int16_t denorm_exp2 = 0;
    auto denorm = dccl::decompose_float_format(FloatLimits::denorm_min(), denorm_exp);
    auto neg_denorm = dccl::decompose_float_format(-FloatLimits::denorm_min(), denorm_exp2);
    assert(denorm == 1);
    assert(neg_denorm == -1);
    assert(denorm_exp == denorm_exp2);

    // 1.0 is 2^0, so the significand is the implicit bit alone
    int16_t one_exp = 0;
    auto one = dccl::decompose_float_format(static_cast<Float>(1.0), one_exp);
    assert(one > 0);
    assert(std::ldexp(static_cast<Float>(one), one_exp) == static_cast<Float>(1.0));

    // sign is carried by the significand, not the exponent
    int16_t pos_exp = 0;
    int16_t neg_exp = 0;
    auto pos = dccl::decompose_float_format(static_cast<Float>(3.25), pos_exp);
    auto neg = dccl::decompose_float_format(static_cast<Float>(-3.25), neg_exp);
    assert(pos == -neg);
    assert(pos_exp == neg_exp);
}

template <std::size_t N> std::bitset<N> bits_of(unsigned long long value)
{
    return std::bitset<N>(value);
}

void check_bitset_arithmetic()
{
    constexpr std::size_t N = 16;

    auto one = bits_of<N>(1);
    dccl::increment(one);
    assert(one.to_ulong() == 2);

    // increment must carry across the whole width and wrap to zero
    auto all_ones = bits_of<N>(0xFFFF);
    dccl::increment(all_ones);
    assert(all_ones.to_ulong() == 0);

    assert(!dccl::is_negative(bits_of<N>(0x7FFF)));
    assert(dccl::is_negative(bits_of<N>(0x8000)));

    auto five = bits_of<N>(5);
    dccl::negate(five);
    assert(five.to_ulong() == 0xFFFB); // two's complement -5
    assert(dccl::negated(bits_of<N>(0)).to_ulong() == 0);

    auto acc = bits_of<N>(7);
    dccl::add_to(acc, bits_of<N>(9));
    assert(acc.to_ulong() == 16);

    assert(dccl::sum(bits_of<N>(1000), bits_of<N>(2345)).to_ulong() == 3345);
    assert(dccl::sum(bits_of<N>(0xFFFF), bits_of<N>(1)).to_ulong() == 0); // wraps
    assert(dccl::difference(bits_of<N>(100), bits_of<N>(58)).to_ulong() == 42);

    assert(dccl::unsigned_geq(bits_of<N>(5), bits_of<N>(5)));
    assert(dccl::unsigned_geq(bits_of<N>(6), bits_of<N>(5)));
    assert(!dccl::unsigned_geq(bits_of<N>(4), bits_of<N>(5)));

    assert(dccl::unsigned_quotient(bits_of<N>(100), bits_of<N>(10)).to_ulong() == 10);
    assert(dccl::unsigned_quotient(bits_of<N>(0), bits_of<N>(7)).to_ulong() == 0);
    // quotient rounds rather than truncating: 7/2 == 3.5 -> 4
    assert(dccl::unsigned_quotient(bits_of<N>(7), bits_of<N>(2)).to_ulong() == 4);
    assert(dccl::unsigned_quotient(bits_of<N>(5), bits_of<N>(2)).to_ulong() == 3);

    assert(dccl::unsigned_product(bits_of<N>(12), bits_of<N>(12)).to_ulong() == 144);
    assert(dccl::unsigned_product(bits_of<N>(0), bits_of<N>(9999)).to_ulong() == 0);
    // the double-width result is what keeps a full-range product from overflowing
    assert(dccl::unsigned_product(bits_of<N>(0xFFFF), bits_of<N>(0xFFFF)).to_ullong() ==
           0xFFFE0001ULL);

    auto shift = bits_of<N>(8);
    dccl::rounding_shift_right(shift, 2);
    assert(shift.to_ulong() == 2);

    // shifting out a set most-significant bit rounds up
    auto shift_round = bits_of<N>(6);
    dccl::rounding_shift_right(shift_round, 2);
    assert(shift_round.to_ulong() == 2);

    auto shift_round_up = bits_of<N>(7);
    dccl::rounding_shift_right(shift_round_up, 2);
    assert(shift_round_up.to_ulong() == 2);

    auto shift_round_half = bits_of<N>(3);
    dccl::rounding_shift_right(shift_round_half, 1);
    assert(shift_round_half.to_ulong() == 2);
}
} // namespace

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    check_special_values<float, int32_t>();
    check_special_values<double, int64_t>();

    check_finite_values<float>();
    check_finite_values<double>();

    check_bitset_arithmetic();

    std::cout << "all tests passed" << std::endl;
    return 0;
}
