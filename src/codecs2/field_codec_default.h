// Copyright 2014-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
// implements FieldCodecBase for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include <chrono>
#include <type_traits>

#include <google/protobuf/descriptor.h>

#include "dccl/option_extensions.pb.h"

#include "../binary.h"
#include "../field_codec.h"
#include "../field_codec_fixed.h"
#include "../thread_safety.h"
#include "field_codec_default_message.h"

namespace dccl
{
/// Goby/DCCL version 2 default field codecs
namespace v2
{
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

        dccl::uint64 uint_value = 0;
        if constexpr (std::is_floating_point_v<std::decay_t<WireType>>) {
            // Float cannot compete with the level of detail capable in (u)int32 types
            // so we need to be careful with the computation. Here we perform computations
            // on the significand/mantissa and exponent values separately for as long as we
            // can, then only convert to a the WireType right before rounding.

            int16_t val_exp, res_exp, min_exp;
            auto val_sig_raw = dccl::decompose_float_format(value, val_exp);
            auto res_sig_raw = dccl::decompose_float_format(static_cast<WireType>(res), res_exp);
            auto min_sig_raw = dccl::decompose_float_format(static_cast<WireType>(min()), min_exp);
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
            auto quant_val_sig = dccl::unsigned_divide(val_pos_sig, res_pos_sig);
            auto quant_val_exp = val_exp - res_exp;
            auto quant_min_sig = dccl::unsigned_divide(min_pos_sig, res_pos_sig);
            auto quant_min_exp = min_exp - res_exp;

            // Now we get them to exponent zero, rounding if necessary
            if (quant_val_exp < 0) {
                dccl::rounding_shift_right(quant_val_sig, static_cast<uint16_t>(-quant_val_exp));
            } else {
                quant_val_sig <<= quant_val_exp;
            }
            quant_val_exp = 0;
            if (quant_min_exp < 0) {
                dccl::rounding_shift_right(quant_min_sig, static_cast<uint16_t>(-quant_min_exp));
            } else {
                quant_min_sig <<= quant_min_exp;
            }
            quant_min_exp = 0;

            // Apply signs
            if (val_sign) {
                dccl::negate(quant_val_sig);
            }
            if (min_sign) {
                dccl::negate(quant_min_sig);
            }

            const auto value_enc_bits = dccl::subtract(quant_val_sig, quant_min_sig);
            // Encoding expected to be positive
            assert(!dccl::is_negative(value_enc_bits));

            const auto value_enc = dccl::fill_unsigned<unsigned_sig_t>(value_enc_bits);

            uint_value = static_cast<dccl::uint64>(value_enc);
        } else {
            WireType wire_value = dccl::quantize(value, res);

            // calculate the encoded value: remove the minimum, scale for the resolution, cast to int.
            wire_value -= dccl::quantize(static_cast<WireType>(min()), res);
            if (res >= 1)
                wire_value /= res;
            else
                wire_value *= (1.0 / res);
            uint_value = static_cast<dccl::uint64>(dccl::round(wire_value, 0));
        }

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

        double res = resolution();
        if constexpr (std::is_floating_point_v<std::decay_t<WireType>>) {
            int16_t res_exp, min_exp;
            auto res_sig_raw = dccl::decompose_float_format(static_cast<WireType>(res), res_exp);
            auto min_sig_raw = dccl::decompose_float_format(static_cast<WireType>(min()), min_exp);

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
            const auto val_enc_sig = wider_t{uint_value};
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

            auto quant_min_sig = dccl::unsigned_divide(min_pos_sig, res_pos_sig);
            auto quant_min_exp = min_exp - res_exp;

            if (quant_min_exp < 0) {
                dccl::rounding_shift_right(quant_min_sig, static_cast<uint16_t>(-quant_min_exp));
            } else {
                quant_min_sig <<= quant_min_exp;
            }
            quant_min_exp = 0;

            // Apply signs
            if (min_sign) {
                dccl::negate(quant_min_sig);
            }

            auto sum_pos_bits = dccl::sum(val_enc_sig, quant_min_sig);
            const auto sum_sign = dccl::is_negative(sum_pos_bits);
            if (sum_sign) {
                dccl::negate(sum_pos_bits);
            }
            const auto sum_exp = 0;

            const auto sum_pos_raw_unsigned = dccl::fill_unsigned<unsigned_sig_t>(sum_pos_bits);

            auto value = sum_pos_raw_unsigned * res;
            if (sum_sign) {
                value = -value;
            }

            // round values again to properly handle cases where double precision
            // leads to slightly off values (e.g. 2.099999999 instead of 2.1)
            value = dccl::quantize(value , res);

            return value;
        } else {
            auto wire_value = (WireType)uint_value;
            if (res >= 1)
                wire_value *= res;
            else
                wire_value /= (1.0 / res);

            // round values again to properly handle cases where double precision
            // leads to slightly off values (e.g. 2.099999999 instead of 2.1)
            wire_value =
                dccl::quantize(wire_value + dccl::quantize(static_cast<WireType>(min()), res), res);
            return wire_value;
        }
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

/// \brief Provides a bool encoder. Uses 1 bit if field is `required`, 2 bits if `optional`
///
/// [presence bit (0 bits if required, 1 bit if optional)][value (1 bit)]
class DefaultBoolCodec : public TypedFixedFieldCodec<bool>
{
  public:
    Bitset encode(const bool& wire_value) override;
    Bitset encode() override;
    bool decode(Bitset* bits) override;
    unsigned size() override;
    unsigned size(const bool& wire_value) override { return size(); }
    void validate() override;
};

/// \brief Provides an variable length ASCII string encoder. Can encode strings up to 255 bytes by using a length byte preceeding the string.
///
/// [length of following string (1 byte)][string (0-255 bytes)]
class DefaultStringCodec : public TypedFieldCodec<std::string>
{
  private:
    Bitset encode() override;
    Bitset encode(const std::string& wire_value) override;
    std::string decode(Bitset* bits) override;
    unsigned size() override;
    unsigned size(const std::string& wire_value) override;
    unsigned max_size() override;
    unsigned min_size() override;
    void validate() override;

  private:
    enum
    {
        MAX_STRING_LENGTH = 255
    };
};

/// \brief Provides an fixed length byte string encoder.
class DefaultBytesCodec : public TypedFieldCodec<std::string>
{
  public:
    Bitset encode() override;
    Bitset encode(const std::string& wire_value) override;
    std::string decode(Bitset* bits) override;
    unsigned size() override;
    unsigned size(const std::string& wire_value) override;
    unsigned max_size() override;
    unsigned min_size() override;
    void validate() override;
};

/// \brief Provides an enum encoder. This converts the enumeration to an integer (based on the enumeration <i>index</i> (<b>not</b> its <i>value</i>) and uses DefaultNumericFieldCodec to encode the integer.
class DefaultEnumCodec
    : public DefaultNumericFieldCodec<int32, const google::protobuf::EnumValueDescriptor*>
{
  public:
    int32 pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value) override;
    const google::protobuf::EnumValueDescriptor* post_decode(const int32& wire_value) override;

  private:
    void validate() override {}
    std::size_t hash() override
    {
        return std::hash<std::string>{}(this_field()->enum_type()->DebugString());
    }

    double max() override
    {
        const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
        return e->value_count() - 1;
    }
    double min() override { return 0; }
};

class TimeCodecClock
{
  public:
    TimeCodecClock()
    {
        // if no other clock, set std::chrono::system_clock as clock
        if (!this->has_clock())
            this->set_clock<std::chrono::system_clock>();
    }

    template <typename Clock> static void set_clock()
    {
#if DCCL_THREAD_SUPPORT
        const std::lock_guard<std::mutex> lock(clock_mutex_);
#endif

        epoch_sec_func_ = []() -> int64
        {
            typename Clock::time_point now = Clock::now();
            std::chrono::seconds sec =
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
            return sec.count();
        };
    }
    static bool has_clock()
    {
#if DCCL_THREAD_SUPPORT
        const std::lock_guard<std::mutex> lock(clock_mutex_);
#endif
        return epoch_sec_func_ ? true : false;
    }

  protected:
    static int64 epoch_sec()
    {
#if DCCL_THREAD_SUPPORT
        const std::lock_guard<std::mutex> lock(clock_mutex_);
#endif
        return epoch_sec_func_();
    }

  private:
#if DCCL_THREAD_SUPPORT
    static std::mutex clock_mutex_;
#endif
    static std::function<int64()> epoch_sec_func_;
};

typedef double time_wire_type;
/// \brief Encodes time of day (default: second precision, but can be set with (dccl.field).precision extension)
///
/// \tparam TimeType A type representing time: See the various specializations of this class for allowed types.
template <typename TimeType, int conversion_factor>
class TimeCodecBase : public DefaultNumericFieldCodec<time_wire_type, TimeType>,
                      public TimeCodecClock
{
  public:
    TimeCodecBase() {}

    time_wire_type pre_encode(const TimeType& time_of_day) override
    {
        time_wire_type max_secs = max();
        return std::fmod(time_of_day / static_cast<time_wire_type>(conversion_factor), max_secs);
    }

    TimeType post_decode(const time_wire_type& encoded_time) override
    {
        auto max_secs = static_cast<int64>(max());
        int64_t now_secs = this->epoch_sec();
        int64 daystart = now_secs - (now_secs % max_secs);
        int64 today_time = now_secs - daystart;

        // If time is more than 12 hours ahead of now, assume it's yesterday.
        if ((encoded_time - today_time) > (max_secs / 2))
            daystart -= max_secs;
        else if ((today_time - encoded_time) > (max_secs / 2))
            daystart += max_secs;

        return dccl::round((TimeType)(conversion_factor * (daystart + encoded_time)),
                           precision() - std::log10((double)conversion_factor));
    }

  private:
    void validate() override
    {
        DefaultNumericFieldCodec<time_wire_type, TimeType>::validate_numeric_bounds();
    }

    double max() override
    {
        return FieldCodecBase::dccl_field_options().num_days() * SECONDS_IN_DAY;
    }

    double min() override { return 0; }
    double precision() override
    {
        if (!FieldCodecBase::dccl_field_options().has_precision())
            return 0; // default to second precision
        else
        {
            return FieldCodecBase::dccl_field_options().precision() +
                   (double)std::log10((double)conversion_factor);
        }
    }

  private:
    enum
    {
        SECONDS_IN_DAY = 86400
    };
};

template <typename TimeType> class TimeCodec : public TimeCodecBase<TimeType, 0>
{
    static_assert(sizeof(TimeCodec) == 0, "Must use specialization of TimeCodec");
};

template <> class TimeCodec<uint64> : public TimeCodecBase<uint64, 1000000>
{
};
template <> class TimeCodec<int64> : public TimeCodecBase<int64, 1000000>
{
};
template <> class TimeCodec<double> : public TimeCodecBase<double, 1>
{
};

/// \brief Placeholder codec that takes no space on the wire (0 bits).
template <typename T> class StaticCodec : public TypedFixedFieldCodec<T>
{
    Bitset encode(const T&) override { return Bitset(size()); }

    Bitset encode() override { return Bitset(size()); }

    T decode(Bitset* /*bits*/) override
    {
        std::istringstream iss(FieldCodecBase::dccl_field_options().static_value());
        T value;
        iss >> value;
        return value;
    }

    unsigned size() override { return 0; }

    void validate() override
    {
        FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_static_value(),
                                "missing (dccl.field).static_value");

        std::string t = FieldCodecBase::dccl_field_options().static_value();
        std::istringstream iss(t);
        T value;

        if (!(iss >> value))
        {
            FieldCodecBase::require(false, "invalid static_value for this type.");
        }
    }
};
} // namespace v2
} // namespace dccl

#endif
