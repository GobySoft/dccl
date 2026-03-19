// Copyright 2009-2024:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
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
// implements FieldCodecBase for all the basic DCCL types for version 5

#ifndef DCCLV5FIELDCODECDEFAULT20240101H
#define DCCLV5FIELDCODECDEFAULT20240101H

#include "../codecs4/field_codec_default.h"

#include <type_traits>
#include "../numeric.h"

namespace dccl
{
/// DCCL version 5 default field codecs
namespace v5
{

// v5 DefaultNumericFieldCodec is the same fundamental algorithm as v2-v4 but with fixed numeric precision at edge cases: https://github.com/GobySoft/dccl/pull/152

/// \brief Provides a basic bounded arbitrary length numeric (double, float, uint32, uint64, int32, int64) encoder.
///
/// Takes ceil(log2((max-min)*10^precision)+1) bits for required fields, ceil(log2((max-min)*10^precision)+2) for optional fields.
template <typename WireType, typename FieldType = WireType>
class DefaultNumericFieldCodec : public TypedFixedFieldCodec<WireType, FieldType>
{
  public:
    virtual double max()
    {
        DynamicConditions& dc = this->dynamic_conditions(this->this_field());

        double static_max = this->dccl_field_options().max();
        if (dc.has_max())
        {
            dc.regenerate(this->this_message(), this->root_message());
            // don't let dynamic conditions breach static bounds
            return std::max(this->dccl_field_options().min(), std::min(dc.max(), static_max));
        }
        else
        {
            return static_max;
        }
    }

    virtual double min()
    {
        DynamicConditions& dc = this->dynamic_conditions(this->this_field());
        double static_min = this->dccl_field_options().min();
        if (dc.has_min())
        {
            dc.regenerate(this->this_message(), this->root_message());

            // don't let dynamic conditions breach static bounds
            return std::min(this->dccl_field_options().max(), std::max(dc.min(), static_min));
        }
        else
        {
            return static_min;
        }
    }

    virtual double precision() { return FieldCodecBase::dccl_field_options().precision(); }

    virtual double resolution()
    {
        if (FieldCodecBase::dccl_field_options().has_precision())
            return std::pow(10.0, -precision());
        // If none is set returns the default resolution (=1)
        return FieldCodecBase::dccl_field_options().resolution();
    }

    void validate() override
    {
        FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_min(),
                                "missing (dccl.field).min");
        FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_max(),
                                "missing (dccl.field).max");

        FieldCodecBase::require(FieldCodecBase::dccl_field_options().resolution() > 0,
                                "(dccl.field).resolution must be greater than 0");
        FieldCodecBase::require(
            !(FieldCodecBase::dccl_field_options().has_precision() &&
              FieldCodecBase::dccl_field_options().has_resolution()),
            "at most one of either (dccl.field).precision or (dccl.field).resolution can be set");

        validate_numeric_bounds();
    }

    void validate_numeric_bounds()
    {
        // ensure given max and min fit within WireType ranges
        FieldCodecBase::require(static_cast<WireType>(min()) >=
                                    std::numeric_limits<WireType>::lowest(),
                                "(dccl.field).min must be >= minimum of this field type.");
        FieldCodecBase::require(static_cast<WireType>(max()) <=
                                    std::numeric_limits<WireType>::max(),
                                "(dccl.field).max must be <= maximum of this field type.");

        // allowable epsilon for min / max to diverge from nearest quantile
        const double min_max_eps = 1e-10;
        bool min_multiple_of_res = std::abs(quantize(min(), resolution()) - min()) < min_max_eps;
        bool max_multiple_of_res = std::abs(quantize(max(), resolution()) - max()) < min_max_eps;
        if (FieldCodecBase::dccl_field_options().has_resolution())
        {
            // ensure that max and min are multiples of the resolution chosen
            FieldCodecBase::require(
                min_multiple_of_res,
                "(dccl.field).min must be an exact multiple of (dccl.field).resolution");
            FieldCodecBase::require(
                max_multiple_of_res,
                "(dccl.field).max must be an exact multiple of (dccl.field).resolution");
        }
        else
        {
            auto res = resolution();
            // this was previously allowed so we will only give a warning not throw an exception
            if (!min_multiple_of_res)
                dccl::dlog.is(dccl::logger::WARN, dccl::logger::GENERAL) &&
                    dccl::dlog << "Warning: (dccl.field).min should be an exact multiple of "
                                  "10^(-(dccl.field).precision), i.e. "
                               << res << ": " << this->this_field()->DebugString() << std::endl;
            if (!max_multiple_of_res)
                dccl::dlog.is(dccl::logger::WARN, dccl::logger::GENERAL) &&
                    dccl::dlog << "Warning: (dccl.field).max should be an exact multiple of "
                                  "10^(-(dccl.field).precision), i.e. "
                               << res << ": " << this->this_field()->DebugString() << std::endl;
        }

        // ensure value fits into double
        FieldCodecBase::require(std::log2(max() - min()) - std::log2(resolution()) <=
                                    std::numeric_limits<double>::digits,
                                "[(dccl.field).max-(dccl.field).min]/(dccl.field).resolution must "
                                "fit in a double-precision floating point value. Please increase "
                                "min, decrease max, or decrease precision.");
    }

    Bitset encode() override { return Bitset(size()); }

    Bitset encode(const WireType& value) override
    {
        dccl::dlog.is(dccl::logger::DEBUG2, dccl::logger::ENCODE) &&
            dlog << "Encode " << value << " with bounds: [" << min() << "," << max() << "]"
                 << std::endl;

        double res = resolution();
        double tol = res / 2;
        // check bounds in the input space (NaN-correct)
        if (!(min() - tol <= value && value < max() + tol))
        {
            // strict mode
            if (this->strict())
                throw(dccl::OutOfRangeException(
                    std::string("Value exceeds min/max bounds for field: ") +
                        FieldCodecBase::this_field()->DebugString(),
                    this->this_field(), this->this_descriptor()));
            // non-strict (default): if out-of-bounds, send as zeros
            else
                return Bitset(size());
        }

        dccl::uint64 uint_value = dccl::encode(value, min(), res);

        // "presence" value (0)
        if (!FieldCodecBase::use_required())
            uint_value += 1;

        Bitset encoded;
        encoded.from(uint_value, size());
        return encoded;
    }

    WireType decode(Bitset* bits) override
    {
        dccl::dlog.is(dccl::logger::DEBUG2, dccl::logger::DECODE) &&
            dlog << "Decode with bounds: [" << min() << "," << max() << "]" << std::endl;

        // The line below SHOULD BE:
        // dccl::uint64 t = bits->to<dccl::uint64>();
        // But GCC3.3 requires an explicit template modifier on the method.
        // See, e.g., http://gcc.gnu.org/bugzilla/show_bug.cgi?id=10959
        dccl::uint64 uint_value = (bits->template to<dccl::uint64>)();

        if (!FieldCodecBase::use_required())
        {
            if (!uint_value)
                throw NullValueException();
            --uint_value;
        }

        return dccl::decode<WireType>(uint_value, min(), resolution());
    }

    // bring size(const WireType&) into scope so callers can access it
    using TypedFixedFieldCodec<WireType, FieldType>::size;

    unsigned size() override
    {
        // if not required field, leave one value for unspecified (always encoded as 0)
        unsigned NULL_VALUE = FieldCodecBase::use_required() ? 0 : 1;

        return dccl::ceil_log2((max() - min()) / resolution() + 1 + NULL_VALUE);
    }
};

// all these are the same as version 4
using DefaultBoolCodec = v4::DefaultBoolCodec;
using DefaultEnumCodec = v4::DefaultEnumCodec;

using DefaultBytesCodec = v4::DefaultBytesCodec;
using DefaultStringCodec = v4::DefaultStringCodec;

template <typename TimeType> using TimeCodec = v4::TimeCodec<TimeType>;
template <typename T> using StaticCodec = v4::StaticCodec<T>;

} // namespace v5
} // namespace dccl

#endif
