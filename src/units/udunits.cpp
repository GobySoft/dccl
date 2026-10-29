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

#include "udunits.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <utility>

#if DCCL_THREAD_SUPPORT
#include <mutex>
#endif

#include "dccl/exception.h"
#include "dccl/option_extensions.pb.h"
#include "udunits_special_units.h"

namespace
{
#if DCCL_THREAD_SUPPORT
// ut_parse() isn't reentrant, and this also guards the converter cache
std::mutex udunits_mutex;
#define DCCL_LOCK_UDUNITS_MUTEX std::lock_guard<std::mutex> l(udunits_mutex);
#else
#define DCCL_LOCK_UDUNITS_MUTEX
#endif

const dccl::units::udunits::UnitSystem& unit_system()
{
    static const dccl::units::udunits::UnitSystem system;
    return system;
}

dccl::units::udunits::UnitPtr parse(const std::string& unit)
{
    try
    {
        return unit_system().parse(unit);
    }
    catch (const dccl::Exception&)
    {
        throw;
    }
    catch (const std::exception& e)
    {
        throw dccl::Exception(e.what());
    }
}

// Converters are cached by (from, to), since parsing is far slower than converting. The cache is
// cleared once it grows past this, so callers that convert to arbitrary unit strings don't grow it
// without bound; converters in use stay alive through their shared_ptr.
constexpr std::size_t max_cached_converters = 256;

using ConverterPtr = std::shared_ptr<cv_converter>;

ConverterPtr converter(const std::string& from, const std::string& to)
{
    static std::map<std::pair<std::string, std::string>, ConverterPtr> converters;
    DCCL_LOCK_UDUNITS_MUTEX

    const auto key = std::make_pair(from, to);
    auto it = converters.find(key);
    if (it != converters.end())
        return it->second;

    const auto from_unit = parse(from);
    const auto to_unit = parse(to);
    if (!ut_are_convertible(from_unit.get(), to_unit.get()))
        throw dccl::Exception("\"" + from + "\" can't be converted to \"" + to + "\"");
    ConverterPtr converter(ut_get_converter(from_unit.get(), to_unit.get()), &cv_free);
    if (!converter)
        throw dccl::Exception("Failed to convert \"" + from + "\" to \"" + to + "\"");
    if (converters.size() >= max_cached_converters)
        converters.clear();
    return converters.emplace(key, std::move(converter)).first->second;
}

std::string field_unit(const google::protobuf::FieldDescriptor* field)
{
    const std::string unit = dccl::units::udunits::unit_of(field);
    if (unit.empty())
        throw dccl::Exception("Field " + std::string(field->full_name()) +
                              " has no (dccl.field).udunits");
    return unit;
}

void check_index(const google::protobuf::Message& msg,
                 const google::protobuf::FieldDescriptor* field, int index, bool allow_add)
{
    if (!field->is_repeated())
    {
        if (index != -1)
            throw dccl::Exception("Index " + std::to_string(index) +
                                  " given for non-repeated field " +
                                  std::string(field->full_name()));
        return;
    }
    if (allow_add && index == -1)
        return;
    const int size = msg.GetReflection()->FieldSize(msg, field);
    if (index < 0 || index >= size)
        throw dccl::Exception("Index " + std::to_string(index) + " is out of range for field " +
                              std::string(field->full_name()) + " of size " + std::to_string(size));
}
} // namespace

double dccl::units::udunits::convert(double value, const std::string& from, const std::string& to)
{
    // cv_convert_double() only reads the converter, so it runs outside the lock
    return cv_convert_double(converter(from, to).get(), value);
}

template <typename Int> Int dccl::units::udunits::round_to(double value, const std::string& field)
{
    const double rounded = std::round(value);
    // 2^digits is max + 1, which a double holds exactly; NaN fails both comparisons
    const double past_max = std::ldexp(1.0, std::numeric_limits<Int>::digits);
    if (!(rounded >= static_cast<double>(std::numeric_limits<Int>::min()) && rounded < past_max))
        throw dccl::Exception("Value " + std::to_string(value) +
                              " is out of range for integer field " + field);
    return static_cast<Int>(rounded);
}

template std::int32_t dccl::units::udunits::round_to<std::int32_t>(double, const std::string&);
template std::int64_t dccl::units::udunits::round_to<std::int64_t>(double, const std::string&);
template std::uint32_t dccl::units::udunits::round_to<std::uint32_t>(double, const std::string&);
template std::uint64_t dccl::units::udunits::round_to<std::uint64_t>(double, const std::string&);

bool dccl::units::udunits::convertible(const std::string& from, const std::string& to)
{
    DCCL_LOCK_UDUNITS_MUTEX
    const auto from_unit = parse(from);
    const auto to_unit = parse(to);
    return ut_are_convertible(from_unit.get(), to_unit.get()) != 0;
}

std::string dccl::units::udunits::unit_of(const google::protobuf::FieldDescriptor* field)
{
    const auto& options = field->options().GetExtension(dccl::field);
    return options.has_udunits() ? options.udunits() : std::string();
}

double dccl::units::udunits::get(const google::protobuf::Message& msg,
                                 const google::protobuf::FieldDescriptor* field,
                                 const std::string& unit, int index)
{
    const std::string from = field_unit(field);
    check_index(msg, field, index, false);
    const auto* reflection = msg.GetReflection();
    const bool repeated = field->is_repeated();

    double value = 0;
    switch (field->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            value = repeated ? reflection->GetRepeatedInt32(msg, field, index)
                             : reflection->GetInt32(msg, field);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            value = repeated ? reflection->GetRepeatedInt64(msg, field, index)
                             : reflection->GetInt64(msg, field);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            value = repeated ? reflection->GetRepeatedUInt32(msg, field, index)
                             : reflection->GetUInt32(msg, field);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            value = repeated ? reflection->GetRepeatedUInt64(msg, field, index)
                             : reflection->GetUInt64(msg, field);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            value = repeated ? reflection->GetRepeatedDouble(msg, field, index)
                             : reflection->GetDouble(msg, field);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            value = repeated ? reflection->GetRepeatedFloat(msg, field, index)
                             : reflection->GetFloat(msg, field);
            break;
        default:
            throw dccl::Exception("Field " + std::string(field->full_name()) + " is not numeric");
    }
    return convert(value, from, unit);
}

void dccl::units::udunits::set(google::protobuf::Message* msg,
                               const google::protobuf::FieldDescriptor* field, double value,
                               const std::string& unit, int index)
{
    const std::string to = field_unit(field);
    check_index(*msg, field, index, true);
    const auto* reflection = msg->GetReflection();
    const bool repeated = field->is_repeated();
    const bool add = repeated && index < 0;

    const double converted = convert(value, unit, to);
    const std::string name(field->full_name());

    switch (field->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            const auto v = round_to<std::int32_t>(converted, name);
            add        ? reflection->AddInt32(msg, field, v)
            : repeated ? reflection->SetRepeatedInt32(msg, field, index, v)
                       : reflection->SetInt32(msg, field, v);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            const auto v = round_to<std::int64_t>(converted, name);
            add        ? reflection->AddInt64(msg, field, v)
            : repeated ? reflection->SetRepeatedInt64(msg, field, index, v)
                       : reflection->SetInt64(msg, field, v);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            const auto v = round_to<std::uint32_t>(converted, name);
            add        ? reflection->AddUInt32(msg, field, v)
            : repeated ? reflection->SetRepeatedUInt32(msg, field, index, v)
                       : reflection->SetUInt32(msg, field, v);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            const auto v = round_to<std::uint64_t>(converted, name);
            add        ? reflection->AddUInt64(msg, field, v)
            : repeated ? reflection->SetRepeatedUInt64(msg, field, index, v)
                       : reflection->SetUInt64(msg, field, v);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            add        ? reflection->AddDouble(msg, field, converted)
            : repeated ? reflection->SetRepeatedDouble(msg, field, index, converted)
                       : reflection->SetDouble(msg, field, converted);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            const auto v = static_cast<float>(converted);
            add        ? reflection->AddFloat(msg, field, v)
            : repeated ? reflection->SetRepeatedFloat(msg, field, index, v)
                       : reflection->SetFloat(msg, field, v);
            break;
        }
        default:
            throw dccl::Exception("Field " + std::string(field->full_name()) + " is not numeric");
    }
}
