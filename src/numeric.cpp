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
#include "numeric.h"

namespace dccl
{

int32_t decompose_float_format(float val, int16_t& exponent)
{
    uint32_t& val_bits = reinterpret_cast<uint32_t&>(val);

    constexpr auto num_exp_bits = 8;
    constexpr auto num_frac_bits = 23;
    constexpr auto exponent_bias = 127;

    constexpr auto sign_bit_mask = static_cast<uint32_t>(0b1) << (num_exp_bits + num_frac_bits);
    constexpr auto exp_bits = (static_cast<uint32_t>(1u) << num_exp_bits) - 1;
    constexpr auto exp_bit_mask = exp_bits << num_frac_bits;
    constexpr auto frac_bit_mask = (static_cast<uint32_t>(1u) << num_frac_bits) - 1;

    uint32_t signed_bit = val_bits & sign_bit_mask;
    uint16_t val_exp_bits = static_cast<uint16_t>((val_bits & exp_bit_mask) >> num_frac_bits);
    uint32_t frac_bits = val_bits & frac_bit_mask;
    if (val_exp_bits == exp_bits)
    {
        if (frac_bits == 0)
        {
            // val is an infinity
            if (signed_bit > 0)
            {
                return std::numeric_limits<int32_t>::lowest();
            }
            else
            {
                return std::numeric_limits<int32_t>::max();
            }
        }
        else
        {
            // val is a NaN
            return std::numeric_limits<int32_t>::lowest() + 1;
        }
    }

    if (val_exp_bits == 0)
    {
        if (frac_bits == 0)
        {
            exponent = 0;
            // val is zero
            return 0;
        }
        else
        {
            // val is a subnormal number
            exponent = -exponent_bias + 1 -
                       num_frac_bits; // shift so that the fraction can be interpreted as full integer
            if (signed_bit > 0)
            {
                return -static_cast<int32_t>(frac_bits);
            }
            else
            {
                return static_cast<int32_t>(frac_bits);
            }
        }
    }

    // val is a normal number
    constexpr auto implicit_bit_mask = static_cast<uint32_t>(1u) << num_frac_bits;
    exponent = static_cast<uint32_t>(val_exp_bits) - exponent_bias -
               num_frac_bits; // shift so that the fraction can be interpreted as full integer
    if (signed_bit > 0)
    {
        return -static_cast<int32_t>(frac_bits | implicit_bit_mask);
    }
    else
    {
        return static_cast<int32_t>(frac_bits | implicit_bit_mask);
    }
}

int64_t decompose_float_format(double val, int16_t& exponent)
{
    uint64_t& val_bits = reinterpret_cast<uint64_t&>(val);

    constexpr auto num_exp_bits = 11;
    constexpr auto num_frac_bits = 52;
    constexpr auto exponent_bias = 1023;

    constexpr auto sign_bit_mask = static_cast<uint64_t>(0b1) << (num_exp_bits + num_frac_bits);
    constexpr auto exp_bits = (static_cast<uint64_t>(1u) << num_exp_bits) - 1;
    constexpr auto exp_bit_mask = exp_bits << num_frac_bits;
    constexpr auto frac_bit_mask = (static_cast<uint64_t>(1u) << num_frac_bits) - 1;

    uint64_t signed_bit = val_bits & sign_bit_mask;
    uint16_t val_exp_bits = static_cast<uint16_t>((val_bits & exp_bit_mask) >> num_frac_bits);
    uint64_t frac_bits = val_bits & frac_bit_mask;
    if (val_exp_bits == exp_bits)
    {
        if (frac_bits == 0)
        {
            // val is an infinity
            if (signed_bit > 0)
            {
                return std::numeric_limits<int64_t>::lowest();
            }
            else
            {
                return std::numeric_limits<int64_t>::max();
            }
        }
        else
        {
            // val is a NaN
            return std::numeric_limits<int64_t>::lowest() + 1;
        }
    }

    if (val_exp_bits == 0)
    {
        if (frac_bits == 0)
        {
            exponent = 0;
            // val is zero
            return 0;
        }
        else
        {
            // val is a subnormal number
            exponent = -exponent_bias + 1 -
                       num_frac_bits; // shift so that the fraction can be interpreted as full integer
            if (signed_bit > 0)
            {
                return -static_cast<int64_t>(frac_bits);
            }
            else
            {
                return static_cast<int64_t>(frac_bits);
            }
        }
    }

    // val is a normal number
    constexpr auto implicit_bit_mask = static_cast<uint64_t>(1u) << num_frac_bits;
    exponent = static_cast<uint64_t>(val_exp_bits) - exponent_bias -
               num_frac_bits; // shift so that the fraction can be interpreted as full integer
    if (signed_bit > 0)
    {
        return -static_cast<int64_t>(frac_bits | implicit_bit_mask);
    }
    else
    {
        return static_cast<int64_t>(frac_bits | implicit_bit_mask);
    }
}

float compose_float_format(int32_t significand, int16_t exponent)
{
    if (significand == std::numeric_limits<int32_t>::lowest())
    {
        return -std::numeric_limits<float>::infinity();
    }
    else if (significand == std::numeric_limits<int32_t>::max())
    {
        return std::numeric_limits<float>::infinity();
    }
    else if (significand == std::numeric_limits<int32_t>::lowest() + 1)
    {
        return std::numeric_limits<float>::quiet_NaN();
    }
    else if (significand == 0)
    {
        return 0.0f;
    }

    return ldexp(static_cast<float>(significand), exponent);
}

double compose_float_format(int64_t significand, int16_t exponent)
{
    if (significand == std::numeric_limits<int64_t>::lowest())
    {
        return -std::numeric_limits<double>::infinity();
    }
    else if (significand == std::numeric_limits<int64_t>::max())
    {
        return std::numeric_limits<double>::infinity();
    }
    else if (significand == std::numeric_limits<int64_t>::lowest() + 1)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    else if (significand == 0)
    {
        return 0.0;
    }

    return ldexp(static_cast<double>(significand), exponent);
}

} // namespace dccl
