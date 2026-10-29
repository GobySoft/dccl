// Copyright 2015-2023:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Stephanie Petillo <stephanie@gobysoft.org>
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
#ifndef GenUnitsClassPlugin20150310H
#define GenUnitsClassPlugin20150310H

#include <google/protobuf/descriptor.h>

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////
// Parsing methods for protobuf messages' units fields:
// base_dimensions, derived_dimensions, unit_system
///////////////////////////////////////////////////////////////////////////////////

namespace dccl
{
namespace units
{
// The available base dimensions and their single character abbreviations
inline const std::map<std::string, char>& base_dimension_abbreviations()
{
    static const std::map<std::string, char> dims{
        {"length", 'L'},      {"time", 'T'},        {"mass", 'M'},
        {"plane_angle", 'A'}, {"solid_angle", 'S'}, {"current", 'I'},
        {"temperature", 'K'}, {"amount", 'N'},      {"luminous_intensity", 'J'},
        {"information", 'B'}, {"dimensionless", '-'}};
    return dims;
}

// Returns the base dimension name for an abbreviation, or an empty string if there is none
inline std::string base_dimension_name(char abbreviation)
{
    for (const auto& dim : base_dimension_abbreviations())
        if (dim.second == abbreviation)
            return dim.first;
    return std::string();
}

inline std::string replace_all_copy(std::string s, const std::string& from, const std::string& to)
{
    for (std::string::size_type pos = s.find(from); pos != std::string::npos;
         pos = s.find(from, pos + to.size()))
        s.replace(pos, from.size(), to);
    return s;
}

// Returns the set of base dimension names that the given unit system supports.
// Returns an empty set for general systems (si, cgs) that support all dimensions.
// Specialized single-dimension systems (angle, temperature) return only their
// supported base dimensions.
inline std::set<std::string> get_system_supported_base_dimensions(const std::string& sysname)
{
    if (sysname == "angle::degree" || sysname == "degree" || sysname == "angle::gradian" ||
        sysname == "gradian" || sysname == "angle::revolution" || sysname == "revolution")
        return {"plane_angle", "dimensionless"};
    else if (sysname == "temperature::celsius" || sysname == "celsius" ||
             sysname == "temperature::fahrenheit" || sysname == "fahrenheit")
        return {"temperature", "dimensionless"};
    else
        return {}; // empty = general system (si, cgs, angle::radian, etc.)
}

// Validates that all dimension names in dim_names are compatible with the given system.
// Throws std::runtime_error if an incompatible combination is detected.
//
// Specialized systems (angle::degree, temperature::celsius, etc.) only support their
// native base dimension. Compound Boost Units dimension names (e.g. "angular_velocity",
// "pressure") require multiple base dimensions and cannot be used reliably with
// specialized systems because the missing base dimensions are silently treated as
// dimensionless, causing incorrect unit conversions.
inline void validate_dimensions_for_system(const std::vector<std::string>& dim_names,
                                           const std::string& sysname)
{
    const auto supported = get_system_supported_base_dimensions(sysname);
    if (supported.empty())
    {
        // General system — all dimensions are supported, except that Boost Units' own general
        // systems have no unit of information
        const bool boost_general_system = sysname == "si" || sysname == "boost::units::si" ||
                                          sysname == "angle::radian" || sysname == "cgs" ||
                                          sysname == "boost::units::cgs";
        for (const auto& dim : dim_names)
        {
            if (boost_general_system && dim == "information")
                throw std::runtime_error("Dimension 'information' has no unit in system '" +
                                         sysname +
                                         "'. Use (dccl.field).units.unit = \"information::bit\" or "
                                         "\"information::byte\" instead.");
        }
        return;
    }

    for (const auto& dim : dim_names)
    {
        const bool is_base_dim = base_dimension_abbreviations().count(dim) > 0;
        if (!is_base_dim)
        {
            // Compound Boost Units dimension (e.g. "angular_velocity", "pressure").
            // These require multiple base dimensions and cannot work correctly with
            // specialized single-dimension systems.
            throw std::runtime_error(
                "Dimension '" + dim +
                "' is a compound Boost Units dimension that requires multiple base "
                "dimensions, but system '" + sysname +
                "' only supports single-dimension units. Use a "
                "general system such as \"si\" or define a new system that includes the appropriate dimensions.");
        }
        else if (supported.find(dim) == supported.end())
        {
            // Known base dimension but not supported by this specialized system.
            std::string supported_list;
            for (const auto& s : supported)
            {
                if (!supported_list.empty())
                    supported_list += ", ";
                supported_list += "'" + s + "'";
            }
            throw std::runtime_error("Dimension '" + dim + "' is not supported by system '" +
                                     sysname + "'. That system only supports: " + supported_list +
                                     ".");
        }
    }
}

// Parser for base_dimensions input, e.g. "LT^-1" or "length time^-1": a sequence of base
// dimensions (abbreviated or named, optionally suffixed "_base_dimension"), each with an optional
// "^power". Whitespace is allowed between tokens.
template <typename Iterator>
bool parse_base_dimensions(Iterator first, Iterator last, std::vector<double>& base_dim_powers,
                           std::vector<std::string>& base_dim_chars,
                           std::vector<std::string>& base_dim_strings)
{
    const std::string input(first, last);
    std::string::size_type pos = 0;

    auto skip_space = [&]()
    {
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos])))
            ++pos;
    };
    auto consume = [&](const std::string& token)
    {
        if (input.compare(pos, token.size(), token) != 0)
            return false;
        pos += token.size();
        return true;
    };

    bool parsed_dimension = false;
    while (true)
    {
        skip_space();
        if (pos == input.size())
            break;

        const std::string abbreviated_name = base_dimension_name(input[pos]);
        if (!abbreviated_name.empty())
        {
            base_dim_chars.emplace_back(1, input[pos]);
            base_dim_strings.push_back(abbreviated_name);
            ++pos;
        }
        else
        {
            bool found = false;
            for (const auto& dim : base_dimension_abbreviations())
            {
                if (consume(dim.first))
                {
                    base_dim_chars.emplace_back(1, dim.second);
                    base_dim_strings.push_back(dim.first);
                    found = true;
                    break;
                }
            }
            if (!found)
                break;
        }
        parsed_dimension = true;

        skip_space();
        consume("_base_dimension");

        skip_space();
        if (consume("^"))
        {
            const char* power_begin = input.c_str() + pos;
            char* power_end = nullptr;
            const double power = std::strtod(power_begin, &power_end);
            if (power_end == power_begin)
                return false;
            pos += power_end - power_begin;
            base_dim_powers.push_back(power);
        }
        else
        {
            base_dim_powers.push_back(1);
        }
    }

    skip_space();
    return parsed_dimension && pos == input.size();
}

// Parser for derived_dimensions input, e.g. "force/area": Boost Units dimension names (optionally
// suffixed "_dimension") joined by "*" or "/". Names separated only by spaces are multiplied.
template <typename Iterator>
bool parse_derived_dimensions(Iterator first, Iterator last,
                              std::vector<std::string>& derived_dim_operators,
                              std::vector<std::string>& derived_dim_strings)
{
    const std::string input(first, last);
    std::string::size_type pos = 0;

    auto is_name_char = [](char c) { return (c >= 'a' && c <= 'z') || c == '1' || c == '_'; };
    auto skip_space = [&]()
    {
        while (pos < input.size() && input[pos] == ' ') ++pos;
    };

    std::vector<std::string> params;
    while (pos < input.size() && is_name_char(input[pos]))
    {
        const std::string::size_type name_begin = pos;
        while (pos < input.size() && is_name_char(input[pos])) ++pos;
        params.push_back(input.substr(name_begin, pos - name_begin));

        skip_space();
        if (pos < input.size() && (input[pos] == '*' || input[pos] == '/'))
            derived_dim_operators.emplace_back(1, input[pos++]);
        else
            derived_dim_operators.emplace_back("*");
        skip_space();
    }

    // every name is followed by an operator, so the last one joins nothing
    if (!derived_dim_operators.empty())
        derived_dim_operators.pop_back();

    for (auto& param : params)
    {
        std::string::size_type dim_pos = param.find("_dimension");
        if (dim_pos != std::string::npos)
            param = param.substr(0, dim_pos);
        derived_dim_strings.push_back(param);
    }

    return !params.empty() && pos == input.size();
}

// Extract field type name as string
inline std::string get_field_type_name(google::protobuf::FieldDescriptor::CppType type)
{
    switch (type)
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32: return "google::protobuf::int32";
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64: return "google::protobuf::int64";
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: return "google::protobuf::uint32";
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: return "google::protobuf::uint64";
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: return "double";
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: return "float";
        default: return "double";
    }
}

} // namespace units
} // namespace dccl

///////////////////////////////////////////////////////////////////////////////////
// Print units class plugin pieces to some type of ostream
///////////////////////////////////////////////////////////////////////////////////

// Example:
// ClassName (Range)
// fieldname (distance)
// sysname (si)
// dimtype_dimension (length_dimension)

// Boost Units - Dimensions Reference:
// http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Reference.html#dimensions_reference

// Generate necessary units systems headers
inline void include_units_headers(const std::string& sysname, std::ostream& os)
{
    os << std::endl;
    /* os <<std::endl; */
    /* os <<"//===================" <<std::endl; */
    /* os <<std::endl; */

    // pre-defined systems from boost units:
    // http://www.boost.org/doc/libs/1_54_0/boost/units/systems/

    if (sysname == "si" || sysname == "boost::units::si" || sysname == "angle::radian")
    {
        os << "#include <boost/units/systems/si.hpp>" << std::endl;
    }
    else if (sysname == "cgs" || sysname == "boost::units::cgs")
    {
        os << "#include <boost/units/systems/cgs.hpp>" << std::endl;
    }
    else if (sysname == "celsius" || sysname == "boost::units::celsius" ||
             sysname == "temperature::celsius")
    {
        os << "#include <boost/units/systems/temperature/celsius.hpp>" << std::endl;
    }
    else if (sysname == "fahrenheit" || sysname == "boost::units::fahrenheit" ||
             sysname == "temperature::fahrenheit")
    {
        os << "#include <boost/units/systems/temperature/fahrenheit.hpp>" << std::endl;
    }
    else if (sysname == "degree" || sysname == "boost::units::degree" || sysname == "angle::degree")
    {
        os << "#include <boost/units/systems/angle/degrees.hpp>" << std::endl;
    }
    else if (sysname == "gradian" || sysname == "boost::units::gradian" ||
             sysname == "angle::gradian")
    {
        os << "#include <boost/units/systems/angle/gradians.hpp>" << std::endl;
    }
    else if (sysname == "revolution" || sysname == "boost::units::revolution" ||
             sysname == "angle::revolution")
    {
        os << "#include <boost/units/systems/angle/revolutions.hpp>" << std::endl;
    }
    else
    {
        //include necessary non-boost-units system headers
        std::string sysname_sub = dccl::units::replace_all_copy(sysname, "::", "/");
        os << "#include \"" << sysname_sub << ".hpp\"" << std::endl;
    }
}

// Generate necessary units systems headers for an individually defined base unit.
// Unit must be available in this list:
// http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Reference.html#boost_units.Reference.alphabetical_listing_of_base_units
inline void include_base_unit_headers(const std::string& base_unit_category_and_name,
                                      std::ostream& os)
{
    os << std::endl;
    std::string cat_name_sub = dccl::units::replace_all_copy(base_unit_category_and_name, "::", "/");
    os << "#include <boost/units/base_units/" << cat_name_sub << ".hpp>" << std::endl;
}

inline void include_custom_unit_headers(const std::string& header, std::ostream& os)
{
    os << std::endl;
    os << "#include \"" << header << "\"" << std::endl;
}

inline void add_absolute(std::string& before, std::string& after)
{
    before = "boost::units::absolute<" + before;
    after += "> ";
}

inline void add_prefix(const std::string& prefix, std::string& before, std::string& after)
{
    int power = 1;
    if (prefix == "yotta")
        power = 24;
    else if (prefix == "zetta")
        power = 21;
    else if (prefix == "exa")
        power = 18;
    else if (prefix == "peta")
        power = 15;
    else if (prefix == "tera")
        power = 12;
    else if (prefix == "giga")
        power = 9;
    else if (prefix == "mega")
        power = 6;
    else if (prefix == "kilo")
        power = 3;
    else if (prefix == "hecto")
        power = 2;
    else if (prefix == "deka")
        power = 1;
    else if (prefix == "deci")
        power = -1;
    else if (prefix == "centi")
        power = -2;
    else if (prefix == "milli")
        power = -3;
    else if (prefix == "micro")
        power = -6;
    else if (prefix == "nano")
        power = -9;
    else if (prefix == "pico")
        power = -12;
    else if (prefix == "femto")
        power = -15;
    else if (prefix == "atto")
        power = -18;
    else if (prefix == "zepto")
        power = -21;
    else if (prefix == "yocto")
        power = -24;
    else
        throw(std::runtime_error(std::string("Invalid SI prefix: " + prefix)));

    std::stringstream power_ss;
    power_ss << power;

    before = "boost::units::make_scaled_unit<" + before;
    after += ", boost::units::scale<10, boost::units::static_rational<" + power_ss.str() +
             "> > >::type ";
}

// Generate a unit typedef when given derived_ or base_dimensions
inline void construct_units_typedef_from_dimension(const std::string& fieldname,
                                                   const std::string& sysname, const bool& absolute,
                                                   const std::string& prefix, std::ostream& os)
{
    // Namespace the sysname if necessary
    std::string sysname_ns;
    if (sysname == "si" || sysname == "cgs" || sysname == "celsius" || sysname == "fahrenheit" ||
        sysname == "degree" || sysname == "gradian" || sysname == "revolution")
        sysname_ns = "boost::units::" + sysname + "::system";
    else if (sysname == "boost::units::si" || sysname == "boost::units::cgs")
        sysname_ns = sysname + "::system";
    else if (sysname == "temperature::celsius" || sysname == "temperature::fahrenheit")
        sysname_ns = dccl::units::replace_all_copy(sysname, "temperature::", "boost::units::");
    else if (sysname == "angle::degree" || sysname == "angle::gradian" ||
             sysname == "angle::revolution")
        sysname_ns = dccl::units::replace_all_copy(sysname, "angle::", "boost::units::") + "::system";
    else if (sysname == "angle::radian")
        sysname_ns = "boost::units::si::system";
    else
        sysname_ns = sysname;

    std::string before;
    std::string after;

    if (absolute && !prefix.empty())
        throw(std::runtime_error(
            std::string("'prefix' is not supported with an absolute temperature field")));

    if (absolute)
        add_absolute(before, after);

    if (!prefix.empty())
        add_prefix(prefix, before, after);

    // Typedef the unit
    os << "typedef " << before << "boost::units::unit<" << fieldname << "_dimension," << sysname_ns
       << "> " << after << fieldname << "_unit;" << std::endl;
    os << std::endl;
}

// Generate a dimension typedef when given base_dimensions
inline void construct_base_dims_typedef(const std::vector<std::string>& dim_vec,
                                        const std::vector<double>& power_vec,
                                        const std::string& fieldname, const std::string& sysname,
                                        const bool& rel_temperature, const std::string& prefix,
                                        std::ostream& os)
{
    /////////// BASE DIMENSIONS --> DERIVED FIELD DIMENSION
    bool temperature_dimension = false;
    if (dim_vec[0] == "temperature" && dim_vec.size() == 1)
        temperature_dimension = true;
    if (dim_vec[0] == "dimensionless" && dim_vec.size() == 1)
    {
        os << "typedef boost::units::dimensionless_type " << fieldname << "_dimension;"
           << std::endl;
        ;
    }
    else
    {
        os << "typedef boost::units::derived_dimension< ";
        for (std::size_t i = 0, n = dim_vec.size(); i < n; i++)
        {
            os << "boost::units::" << dim_vec[i] << "_base_dimension," << power_vec[i];
            if (i != dim_vec.size() - 1)
                os << ", ";
        }
        os << " >::type " << fieldname << "_dimension;" << std::endl;
    }
    os << std::endl;

    construct_units_typedef_from_dimension(fieldname, sysname,
                                           temperature_dimension && !rel_temperature, prefix, os);
}

// Generate a dimension typedef when given derived_dimensions
inline void construct_derived_dims_typedef(const std::vector<std::string>& dim_vec,
                                           const std::vector<std::string>& operator_vec,
                                           const std::string& fieldname, const std::string& sysname,
                                           const bool& rel_temperature, const std::string& prefix,
                                           std::ostream& os)
{
    /////////// DERIVED DIMENSIONS --> DERIVED FIELD DIMENSION

    bool temperature_dimension = false;

    if (dim_vec.size() == 1)
    {
        if (dim_vec[0] == "temperature")
            temperature_dimension = true;
        if (dim_vec[0] == "dimensionless")
            os << "typedef boost::units::dimensionless_type " << fieldname << "_dimension;"
               << std::endl;
        else
            os << "typedef boost::units::" << dim_vec[0] << "_dimension " << fieldname
               << "_dimension;" << std::endl;
    }
    else
    { //construct new dimension type with mpl divides/times calls based on operators and powers
        //see example here:
        //http://www.boost.org/doc/libs/1_57_0/doc/html/boost_units/Examples.html#boost_units.Examples.DimensionExample
        os << "typedef ";
        std::string result = "boost::units::" + dim_vec[0] + "_dimension";
        for (std::size_t i = 0, n = operator_vec.size(); i < n; i++)
        {
            if (operator_vec[i] == "/")
                result = "boost::mpl::divides<" + result + ",boost::units::" + dim_vec[i + 1] +
                         "_dimension>::type";
            else if (operator_vec[i] == "*")
                result = "boost::mpl::times<" + result + ",boost::units::" + dim_vec[i + 1] +
                         "_dimension>::type";
        }
        os << result << " " << fieldname << "_dimension;" << std::endl;
    }
    os << std::endl;

    construct_units_typedef_from_dimension(fieldname, sysname,
                                           temperature_dimension && !rel_temperature, prefix, os);
}

//=============VVVV

//===========
// Generate a unit typedef when given base_unit
inline void construct_units_typedef_from_base_unit(const std::string& fieldname,
                                                   const std::string& base_unit_category_and_name,
                                                   const bool& rel_temperature,
                                                   const std::string& prefix, std::ostream& os)
{
    bool temperature_unit = false;

    //expect to see "temperature::celsius" or "temperature::fahrenheit" or "si::kelvin"
    if ((base_unit_category_and_name.find("temperature") != std::string::npos) ||
        (base_unit_category_and_name.find("kelvin") != std::string::npos))
        temperature_unit = true;

    bool absolute = temperature_unit && !rel_temperature;

    std::string before;
    std::string after;
    if (absolute)
        add_absolute(before, after);

    if (!prefix.empty())
        add_prefix(prefix, before, after);

    // Namespace and typedef the unit
    os << "typedef " << before << "boost::units::" << base_unit_category_and_name
       << "_base_unit::unit_type " << after << fieldname << "_unit;" << std::endl;

    os << std::endl;
}

//===========
// Generate a unit typedef when given custom unit
inline void construct_units_typedef_from_custom_unit(const std::string& fieldname,
                                                     const std::string& unit_name,
                                                     const bool& rel_temperature,
                                                     const std::string& prefix, std::ostream& os)
{
    bool temperature_unit = false;

    //expect to see "temperature::celsius" or "temperature::fahrenheit" or "si::kelvin"
    if ((unit_name.find("temperature") != std::string::npos) ||
        (unit_name.find("kelvin") != std::string::npos))
        temperature_unit = true;

    bool absolute = temperature_unit && !rel_temperature;

    std::string before;
    std::string after;
    if (absolute)
        add_absolute(before, after);

    if (!prefix.empty())
        add_prefix(prefix, before, after);

    // Namespace and typedef the unit
    os << "typedef " << before << unit_name << " " << after << fieldname << "_unit;" << std::endl;
    os << std::endl;
}

//===========
// Generate the body of the units plugin for the message class
inline void construct_field_class_plugin(const std::string& fieldname, std::ostream& os,
                                         const std::string& value_type, bool is_repeated)
{
    /////////// DERIVED FIELD DIMENSION --> PROTOBUF EXTENSIONS

    // Overloading set_fieldname to accept boost units, i.e. quantity<unit<fieldname_dimension,sysname::system> >
    os << "template<typename Quantity >" << std::endl;
    os << "  void set_" << fieldname << "_with_units(";
    if (is_repeated)
        os << "int index, ";
    os << "Quantity value_w_units)" << std::endl;
    os << "  { set_" << fieldname << "(";
    if (is_repeated)
        os << "index, ";
    os << "boost::units::quantity<" << fieldname << "_unit," << value_type
       << " >(value_w_units).value() ); };" << std::endl;
    os << std::endl;

    if (is_repeated)
    {
        os << "template<typename Quantity >" << std::endl;
        os << "  void add_" << fieldname << "_with_units(Quantity value_w_units)" << std::endl;
        os << "  { add_" << fieldname << "(boost::units::quantity<" << fieldname << "_unit,"
           << value_type << " >(value_w_units).value() ); };" << std::endl;
        os << std::endl;
    }

    //returns whatever units are requested
    os << "template<typename Quantity >" << std::endl;
    os << "  Quantity " << fieldname << "_with_units(";
    if (is_repeated)
        os << "int index";
    os << ") const" << std::endl;
    os << "  { return Quantity(" << fieldname << "(";
    if (is_repeated)
        os << "index";
    os << ") * " << fieldname << "_unit()); };" << std::endl;
    os << std::endl;

    //returns same units only
    os << "boost::units::quantity< " << fieldname << "_unit," << value_type << " > " << fieldname
       << "_with_units(";
    if (is_repeated)
        os << "int index";
    os << ") const" << std::endl;
    os << "  { return " << fieldname << "_with_units<boost::units::quantity< " << fieldname
       << "_unit," << value_type << " > >(";
    if (is_repeated)
        os << "index";
    os << "); };" << std::endl;
    os << std::endl;
}

// Generate the body of the units plugin for a field whose stored value v is the quantity
// (v * scale + offset) in its unit typedef, e.g. a field in knots with an SI unit typedef
inline void construct_converting_field_class_plugin(const std::string& fieldname, std::ostream& os,
                                                    const std::string& field_type, bool is_integer,
                                                    bool is_repeated, double scale, double offset)
{
    // integer fields always take this path, so that they round rather than truncate
    if (scale == 1 && offset == 0 && !is_integer)
    {
        construct_field_class_plugin(fieldname, os, field_type, is_repeated);
        return;
    }

    std::ostringstream constants;
    // the generated code must not depend on the global locale (e.g. "0,5")
    constants.imbue(std::locale::classic());
    constants.precision(17);
    if (scale != 1)
        constants << "static constexpr double " << fieldname << "_si_scale = " << scale << ";"
                  << std::endl;
    if (offset != 0)
        constants << "static constexpr double " << fieldname << "_si_offset = " << offset << ";"
                  << std::endl;
    os << constants.str() << std::endl;

    const std::string quantity = "boost::units::quantity< " + fieldname + "_unit,double >";
    const std::string index_param = is_repeated ? "int index" : "";
    const std::string index_arg = is_repeated ? "index" : "";

    std::string to_field =
        "boost::units::quantity<" + fieldname + "_unit,double >(value_w_units).value()";
    if (offset != 0)
        to_field = "(" + to_field + " - " + fieldname + "_si_offset)";
    if (scale != 1)
        to_field += " / " + fieldname + "_si_scale";
    to_field = is_integer ? "static_cast<" + field_type + ">(std::llround(" + to_field + "))"
                          : "static_cast<" + field_type + ">(" + to_field + ")";

    std::string from_field = fieldname + "(" + index_arg + ")";
    if (scale != 1)
        from_field += " * " + fieldname + "_si_scale";
    if (offset != 0)
        from_field += " + " + fieldname + "_si_offset";
    from_field = "(" + from_field + ")";

    os << "template<typename Quantity >" << std::endl;
    os << "  void set_" << fieldname << "_with_units(" << (is_repeated ? "int index, " : "")
       << "Quantity value_w_units)" << std::endl;
    os << "  { set_" << fieldname << "(" << (is_repeated ? "index, " : "") << to_field << "); };"
       << std::endl;
    os << std::endl;

    if (is_repeated)
    {
        os << "template<typename Quantity >" << std::endl;
        os << "  void add_" << fieldname << "_with_units(Quantity value_w_units)" << std::endl;
        os << "  { add_" << fieldname << "(" << to_field << "); };" << std::endl;
        os << std::endl;
    }

    //returns whatever units are requested
    os << "template<typename Quantity >" << std::endl;
    os << "  Quantity " << fieldname << "_with_units(" << index_param << ") const" << std::endl;
    os << "  { return Quantity(" << from_field << " * " << fieldname << "_unit()); };" << std::endl;
    os << std::endl;

    //returns same units only
    os << quantity << " " << fieldname << "_with_units(" << index_param << ") const" << std::endl;
    os << "  { return " << fieldname << "_with_units<" << quantity << " >(" << index_arg << "); };"
       << std::endl;
    os << std::endl;
}

// Generate accessors that convert a field with (dccl.field).udunits to and from any unit at run
// time, through libdccl's UDUNITS-2 wrapper
inline void construct_udunits_runtime_accessors(const std::string& fieldname,
                                                const std::string& field_full_name,
                                                std::ostream& os, const std::string& field_type,
                                                bool is_integer, bool is_repeated)
{
    const std::string index_param = is_repeated ? "int index, " : "";
    const std::string index_arg = is_repeated ? "index" : "";
    auto to_field = [&](const std::string& value)
    {
        const std::string converted =
            "dccl::units::udunits::convert(" + value + ", unit, " + fieldname + "_udunits())";
        // round_to() throws for a value that doesn't fit the field
        return is_integer ? "dccl::units::udunits::round_to<" + field_type + ">(" + converted +
                                ", \"" + field_full_name + "\")"
                          : "static_cast<" + field_type + ">(" + converted + ")";
    };

    os << "double " << fieldname << "_with_udunits(" << index_param
       << "const std::string& unit) const" << std::endl;
    os << "  { return dccl::units::udunits::convert(" << fieldname << "(" << index_arg << "), "
       << fieldname << "_udunits(), unit); }" << std::endl;
    os << "void set_" << fieldname << "_with_udunits(" << index_param
       << "double value, const std::string& unit)" << std::endl;
    os << "  { set_" << fieldname << "(" << (is_repeated ? "index, " : "") << to_field("value")
       << "); }" << std::endl;
    if (is_repeated)
    {
        os << "void add_" << fieldname << "_with_udunits(double value, const std::string& unit)"
           << std::endl;
        os << "  { add_" << fieldname << "(" << to_field("value") << "); }" << std::endl;
    }
    os << std::endl;
}

//=============^^^^

#endif
