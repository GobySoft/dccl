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

#include "udunits_gen.h"

#include <memory>
#include <stdexcept>

#include <udunits2.h>

namespace
{
// The stock unit database, loaded on first use. UDUNITS2_XML_PATH overrides its location.
ut_system* unit_system()
{
    static ut_system* system = []()
    {
        ut_set_error_message_handler(ut_ignore);
        ut_system* loaded = ut_read_xml(nullptr);
        if (!loaded)
            throw std::runtime_error(
                "Failed to load the UDUNITS-2 unit database (ut_read_xml status " +
                std::to_string(ut_get_status()) +
                "). Set UDUNITS2_XML_PATH to the location of udunits2.xml.");
        return loaded;
    }();
    return system;
}
} // namespace

void dccl::units::udunits::validate(const std::string& unit)
{
    // an empty string parses as "1", which is never what was meant
    std::unique_ptr<ut_unit, decltype(&ut_free)> parsed(
        unit.empty() ? nullptr : ut_parse(unit_system(), unit.c_str(), UT_UTF8), &ut_free);
    if (!parsed)
        throw std::runtime_error("(dccl.field).udunits \"" + unit +
                                 "\" is not a unit that UDUNITS-2 can parse.");
}
