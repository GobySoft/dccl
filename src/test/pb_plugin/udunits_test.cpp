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

// Checks the unit strings generated for (dccl.field).udunits.

#include <cstring>
#include <iostream>
#include <string>

#include "dccl/test/pb_plugin/udunits.pb.h"

namespace
{
int failures = 0;

void check(const std::string& what, const char* actual, const char* expected)
{
    if (std::strcmp(actual, expected) != 0)
    {
        std::cerr << "FAIL: " << what << ": expected \"" << expected << "\", got \"" << actual
                  << "\"" << std::endl;
        ++failures;
    }
    else
    {
        std::cout << "ok: " << what << std::endl;
    }
}
} // namespace

int main()
{
    using dccl::test::plugin::UdunitsMsg;

    check("a named unit", UdunitsMsg::speed_udunits(), "knot");
    check("a unit with an exponent", UdunitsMsg::velocity_udunits(), "m s-1");
    check("a UTF-8 unit", UdunitsMsg::bearing_udunits(), "°");
    check("a unit that needs escaping", UdunitsMsg::bearing_error_udunits(), "\"");
    check("an offset unit", UdunitsMsg::temperature_udunits(), "degC");
    check("an integer field's unit", UdunitsMsg::depth_cm_udunits(), "cm");
    check("a repeated field's unit", UdunitsMsg::depths_udunits(), "m");

    // also callable on an instance
    UdunitsMsg msg;
    check("the unit through an instance", msg.speed_udunits(), "knot");

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
