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

#include "udunits_special_units.h"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
// Units that stock UDUNITS-2 defines in terms of radians, steradians or bits, all of which it
// treats as dimensionless. Each is replaced by its value in reference units, expressed in DCCL's
// base units by replacement.
struct SpecialUnit
{
    std::vector<std::string> names; // names and symbols, as in the stock XML database
    std::string reference;
    std::string replacement;
};

const std::vector<SpecialUnit>& special_units()
{
    static const std::vector<SpecialUnit> units{
        {{"radian",   "rad",          "arc_degree",     "angular_degree", "degree",    "arcdeg",
          "°",        "arc_minute",   "angular_minute", "arcminute",      "arcmin",    "'",
          "′",        "arc_second",   "angular_second", "arcsecond",      "arcsec",    "\"",
          "″",        "grade",        "circle",         "cycle",          "turn",      "revolution",
          "rotation", "degree_north", "degrees_north",  "degree_N",       "degrees_N", "degreeN",
          "degreesN", "degree_east",  "degrees_east",   "degree_E",       "degrees_E", "degreeE",
          "degreesE", "degree_true",  "degrees_true",   "degree_T",       "degrees_T", "degreeT",
          "degreesT", "degree_west",  "degrees_west",   "degree_W",       "degrees_W", "degreeW",
          "degreesW"},
         "rad",
         "dccl_plane_angle"},
        {{"rotation_per_second", "rps", "cps", "rpm"}, "rad/s", "dccl_plane_angle/s"},
        {{"steradian", "sr"}, "sr", "dccl_solid_angle"},
        {{"lumen", "lm"}, "lm", "cd dccl_solid_angle"},
        {{"lux", "lx", "footcandle", "phot", "ph"}, "lm/m^2", "cd dccl_solid_angle/m^2"},
        {{"bit", "byte", "octet"}, "bit", "dccl_information"},
        // stock UDUNITS-2 makes these synonyms for hertz
        {{"bps", "Bd", "baud"}, "Hz", "dccl_information/s"}};
    return units;
}

// SI prefix names and symbols, as in the stock XML database
const std::vector<std::string>& prefixes()
{
    static const std::vector<std::string> prefixes{
        "yotta", "Y", "zetta", "Z", "exa",   "E", "peta", "P",  "tera", "T", "giga",  "G",
        "mega",  "M", "kilo",  "k", "hecto", "h", "deka", "da", "deci", "d", "centi", "c",
        "milli", "m", "micro", "µ", "μ",     "u", "nano", "n",  "pico", "p", "femto", "f",
        "atto",  "a", "zepto", "z", "yocto", "y"};
    return prefixes;
}

bool matches(const SpecialUnit& special, const std::string& token)
{
    auto is_name = [&](const std::string& s)
    { return std::find(special.names.begin(), special.names.end(), s) != special.names.end(); };
    // UDUNITS-2 also accepts the English plural of a name ("degrees", "luxes")
    auto is_name_or_plural = [&](const std::string& s)
    {
        if (is_name(s))
            return true;
        for (const std::string suffix : {"s", "es"})
        {
            if (s.size() > suffix.size() &&
                s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0 &&
                is_name(s.substr(0, s.size() - suffix.size())))
                return true;
        }
        return false;
    };

    if (is_name_or_plural(token))
        return true;
    for (const auto& prefix : prefixes())
    {
        if (token.size() > prefix.size() && token.compare(0, prefix.size(), prefix) == 0 &&
            is_name_or_plural(token.substr(prefix.size())))
            return true;
    }
    return false;
}

// Superscript digits and signs (UTF-8), which UDUNITS-2 reads as an exponent. Returns the
// number of bytes at pos that form one, and sets ascii to its plain equivalent.
std::size_t superscript_at(const std::string& s, std::size_t pos, char& ascii)
{
    static const std::vector<std::pair<std::string, char>> superscripts{
        {"⁰", '0'}, {"¹", '1'}, {"²", '2'}, {"³", '3'}, {"⁴", '4'}, {"⁵", '5'},
        {"⁶", '6'}, {"⁷", '7'}, {"⁸", '8'}, {"⁹", '9'}, {"⁺", '+'}, {"⁻", '-'}};
    for (const auto& superscript : superscripts)
    {
        if (s.compare(pos, superscript.first.size(), superscript.first) == 0)
        {
            ascii = superscript.second;
            return superscript.first.size();
        }
    }
    return 0;
}

// The UTF-8 multiplication sign "·" (U+00B7), which UDUNITS-2 accepts between units
bool is_middle_dot(const std::string& s, std::size_t pos)
{
    return s.compare(pos, 2, "\xC2\xB7") == 0 ||
           (pos > 0 && s.compare(pos - 1, 2, "\xC2\xB7") == 0);
}

bool is_identifier_byte(const std::string& s, std::size_t pos)
{
    const auto c = static_cast<unsigned char>(s[pos]);
    char ascii;
    return std::isalpha(c) || c == '_' ||
           (c >= 0x80 && superscript_at(s, pos, ascii) == 0 && !is_middle_dot(s, pos));
}

dccl::units::udunits::UnitPtr parse_stock(ut_system* system, const std::string& unit)
{
    return {unit.empty() ? nullptr : ut_parse(system, unit.c_str(), UT_UTF8), &ut_free};
}
} // namespace

dccl::units::udunits::UnitSystem::UnitSystem()
{
    ut_set_error_message_handler(ut_ignore);
    system_ = ut_read_xml(nullptr);
    if (!system_)
        throw std::runtime_error("Failed to load the UDUNITS-2 unit database (ut_read_xml status " +
                                 std::to_string(ut_get_status()) +
                                 "). Set UDUNITS2_XML_PATH to the location of udunits2.xml.");

    auto add_base_unit = [this](const char* name)
    {
        ut_unit* unit = ut_new_base_unit(system_);
        if (!unit || ut_map_name_to_unit(name, UT_ASCII, unit) != UT_SUCCESS)
            throw std::runtime_error(std::string("Failed to add the base unit ") + name +
                                     " to the UDUNITS-2 unit system");
        return unit;
    };
    plane_angle_ = add_base_unit("dccl_plane_angle");
    solid_angle_ = add_base_unit("dccl_solid_angle");
    information_ = add_base_unit("dccl_information");
}

dccl::units::udunits::UnitSystem::~UnitSystem()
{
    ut_free(plane_angle_);
    ut_free(solid_angle_);
    ut_free(information_);
    ut_free_system(system_);
}

std::string dccl::units::udunits::UnitSystem::rewrite(const std::string& unit) const
{
    std::string rewritten;
    std::size_t pos = 0;
    while (pos < unit.size())
    {
        std::size_t end = pos;
        if (unit[pos] == '\'' || unit[pos] == '"')
            ++end;
        else
            while (end < unit.size() && is_identifier_byte(unit, end)) ++end;

        if (end == pos)
        {
            rewritten += unit[pos++];
            continue;
        }

        const std::string token = unit.substr(pos, end - pos);
        pos = end;

        if (token == "ppt" || token == "pptv")
            throw std::runtime_error(
                "\"" + token +
                "\" is ambiguous: UDUNITS-2 reads it as parts per trillion (1e-12), but it is "
                "often used for parts per thousand. Write \"1e-3\" for parts per thousand or "
                "\"1e-12\" for parts per trillion. For practical salinity (PSS-78), write "
                "\"1\".");

        // not UDUNITS-2 units, but common enough to deserve a better error than a parse failure
        if (token == "PSU" || token == "psu" || token == "PSS" || token == "pss")
            throw std::runtime_error(
                "\"" + unit +
                "\" is not a UDUNITS-2 unit. Practical salinity (PSS-78) is dimensionless: write "
                "\"1\", as the CF conventions do for sea_water_practical_salinity.");

        const auto special = std::find_if(special_units().begin(), special_units().end(),
                                          [&](const SpecialUnit& s) { return matches(s, token); });
        if (special == special_units().end())
        {
            rewritten += token;
            continue;
        }
        UnitPtr from = parse_stock(system_, token);
        if (!from)
        {
            rewritten += token;
            continue;
        }

        UnitPtr to = parse_stock(system_, special->reference);
        cv_converter* converter = ut_get_converter(from.get(), to.get());
        if (!converter)
            throw std::runtime_error("Failed to convert \"" + token + "\" to \"" +
                                     special->reference + "\"");
        std::ostringstream factor;
        // ut_parse() wants "0.5" whatever the global locale says
        factor.imbue(std::locale::classic());
        factor.precision(17);
        factor << cv_convert_double(converter, 1.0);
        cv_free(converter);
        rewritten += "(" + factor.str() + " (" + special->replacement + "))";

        // UDUNITS-2 reads a number after ")" as a factor, not an exponent
        char ascii;
        if (pos < unit.size() &&
            (std::isdigit(static_cast<unsigned char>(unit[pos])) ||
             ((unit[pos] == '-' || unit[pos] == '+') && pos + 1 < unit.size() &&
              std::isdigit(static_cast<unsigned char>(unit[pos + 1])))))
        {
            rewritten += '^';
        }
        else if (superscript_at(unit, pos, ascii))
        {
            rewritten += '^';
            for (std::size_t n; (n = superscript_at(unit, pos, ascii)) > 0; pos += n)
                rewritten += ascii;
        }
    }
    return rewritten;
}

dccl::units::udunits::UnitPtr dccl::units::udunits::UnitSystem::parse(const std::string& unit) const
{
    // an empty string parses as "1", which is never what was meant
    UnitPtr parsed = parse_stock(system_, unit.empty() ? std::string() : rewrite(unit));
    if (!parsed)
        throw std::runtime_error("\"" + unit + "\" is not a unit that UDUNITS-2 can parse.");
    return parsed;
}
