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

// Tests DCCL's special cases on top of the stock UDUNITS-2 unit database.

#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>

#include "dccl/units/udunits_special_units.h"

namespace
{
int failures = 0;

void check(const std::string& what, bool ok)
{
    if (ok)
    {
        std::cout << "ok: " << what << std::endl;
    }
    else
    {
        std::cerr << "FAIL: " << what << std::endl;
        ++failures;
    }
}

void check_rewrite(const dccl::units::udunits::UnitSystem& system, const std::string& unit,
                   const std::string& expected)
{
    const std::string rewritten = system.rewrite(unit);
    check("\"" + unit + "\" is rewritten as \"" + expected + "\" (got \"" + rewritten + "\")",
          rewritten == expected);
}

void check_convertible(const dccl::units::udunits::UnitSystem& system, const std::string& from,
                       const std::string& to, bool expected)
{
    const auto from_unit = system.parse(from);
    const auto to_unit = system.parse(to);
    check("\"" + from + "\" " + (expected ? "converts" : "doesn't convert") + " to \"" + to + "\"",
          (ut_are_convertible(from_unit.get(), to_unit.get()) != 0) == expected);
}

void check_rejected(const dccl::units::udunits::UnitSystem& system, const std::string& unit,
                    const std::string& expected)
{
    try
    {
        system.parse(unit);
        check("\"" + unit + "\" is rejected", false);
    }
    catch (const std::runtime_error& e)
    {
        check("\"" + unit + "\" is rejected with \"" + expected + "\" (got \"" + e.what() + "\")",
              std::string(e.what()).find(expected) != std::string::npos);
    }
}
} // namespace

int main()
{
    dccl::units::udunits::UnitSystem system;

    // units that aren't special are untouched
    check_rewrite(system, "m/s", "m/s");
    check_rewrite(system, "degC", "degC");
    check_rewrite(system, "°C", "°C");
    check_rewrite(system, "percent", "percent");
    check_rewrite(system, "1e-3", "1e-3");

    // special units are replaced by their value in DCCL's base units
    check_rewrite(system, "rad", "(1 (dccl_plane_angle))");
    check_rewrite(system, "degree/s", "(0.017453292519943295 (dccl_plane_angle))/s");
    check_rewrite(system, "°", "(0.017453292519943295 (dccl_plane_angle))");
    check_rewrite(system, "sr", "(1 (dccl_solid_angle))");
    check_rewrite(system, "bytes/s", "(8 (dccl_information))/s");
    check_rewrite(system, "lux", "(1 (cd dccl_solid_angle/m^2))");
    check_rewrite(system, "bps", "(1 (dccl_information/s))");
    check_rewrite(system, "kbps", "(1000 (dccl_information/s))");
    // "·" is UDUNITS-2's multiplication sign, not part of a name
    check_rewrite(system, "rad·s", "(1 (dccl_plane_angle))·s");
    check_rewrite(system, "s·rad", "s·(1 (dccl_plane_angle))");
    // with SI prefix names and symbols
    check_rewrite(system, "mrad", "(0.001 (dccl_plane_angle))");
    check_rewrite(system, "kilobyte", "(8000 (dccl_information))");
    check_rewrite(system, "kbyte", "(8000 (dccl_information))");
    // an exponent after ")" needs a "^", or UDUNITS-2 reads it as a factor
    check_rewrite(system, "rad2", "(1 (dccl_plane_angle))^2");
    check_rewrite(system, "rad-1", "(1 (dccl_plane_angle))^-1");
    check_rewrite(system, "rad²", "(1 (dccl_plane_angle))^2");

    // angles, solid angles and information aren't dimensionless
    check_convertible(system, "degree", "percent", false);
    check_convertible(system, "degree", "1", false);
    check_convertible(system, "byte/s", "Hz", false);
    check_convertible(system, "sr", "rad2", false);
    check_convertible(system, "lux", "cd/m2", false);
    check_convertible(system, "bps", "Hz", false);
    check_convertible(system, "rad·s", "s", false);
    check_convertible(system, "cd·sr", "cd", false);
    check_convertible(system, "bit·s-1", "Hz", false);
    // but convert among themselves
    check_convertible(system, "degree", "rad", true);
    check_convertible(system, "°", "arcdeg", true);
    check_convertible(system, "degrees_north", "degree", true);
    check_convertible(system, "rpm", "degree/s", true);
    check_convertible(system, "rad²", "rad2", true);
    check_convertible(system, "lux", "lm/m2", true);
    check_convertible(system, "byte/s", "bit/s", true);
    check_convertible(system, "kbyte", "bits", true);
    check_convertible(system, "bps", "bit/s", true);
    check_convertible(system, "kbps", "kilobyte/s", true);
    check_convertible(system, "baud", "Bd", true);
    check_convertible(system, "rad·s", "rad s", true);
    check_convertible(system, "bit·s-1", "bit/s", true);
    // other units are as in stock UDUNITS-2
    check_convertible(system, "knot", "m/s", true);
    check_convertible(system, "degC", "K", true);
    check_convertible(system, "percent", "ppm", true);

    check_rejected(system, "ppt", "parts per trillion");
    check_rejected(system, "pptv", "parts per trillion");
    check_rejected(system, "ppt/s", "parts per trillion");
    check_rejected(system, "ppt", "For practical salinity (PSS-78), write \"1\"");
    check_rejected(system, "PSU", "Practical salinity (PSS-78) is dimensionless");
    check_rejected(system, "psu", "Practical salinity (PSS-78) is dimensionless");
    check_rejected(system, "PSS-78", "Practical salinity (PSS-78) is dimensionless");
    check_rejected(system, "deg/s", "not a unit that UDUNITS-2 can parse");
    check_rejected(system, "", "not a unit that UDUNITS-2 can parse");

    // the rewritten factor must not follow a global locale that writes "0,5"
    struct CommaDecimal : std::numpunct<char>
    {
        char do_decimal_point() const override { return ','; }
    };
    const std::locale previous = std::locale::global(std::locale(std::locale(), new CommaDecimal));
    check_rewrite(system, "degree/s", "(0.017453292519943295 (dccl_plane_angle))/s");
    check_convertible(system, "degree", "rad", true);
    std::locale::global(previous);

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
