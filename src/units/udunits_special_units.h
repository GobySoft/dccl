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

#ifndef DCCL_UNITS_UDUNITS_SPECIAL_UNITS_H
#define DCCL_UNITS_UDUNITS_SPECIAL_UNITS_H

#include <memory>
#include <string>

#include <udunits2.h>

namespace dccl
{
namespace units
{
namespace udunits
{
using UnitPtr = std::unique_ptr<ut_unit, decltype(&ut_free)>;

// The stock UDUNITS-2 unit database with DCCL's special cases applied:
//  - plane angle, solid angle and information are base units (DCCL's own dccl_plane_angle,
//    dccl_solid_angle and dccl_information), as they are in Boost Units. Stock UDUNITS-2 treats
//    them all as dimensionless, so "degree" would convert to "percent" and "byte/s" to "Hz".
//  - "ppt" and "pptv" are rejected: UDUNITS-2 reads them as parts per trillion, but they often
//    mean parts per thousand.
// Not thread safe: ut_parse() is not reentrant.
class UnitSystem
{
  public:
    // Loads the stock database (UDUNITS2_XML_PATH overrides its location). Throws
    // std::runtime_error on failure.
    UnitSystem();
    ~UnitSystem();
    UnitSystem(const UnitSystem&) = delete;
    UnitSystem& operator=(const UnitSystem&) = delete;

    // Parses unit with the special cases applied. Throws std::runtime_error explaining why
    // unit can't be used.
    UnitPtr parse(const std::string& unit) const;

    // The string that parse() hands to ut_parse(): each special unit is replaced by its
    // value in DCCL's base units, e.g. "degree/s" -> "(0.017453292519943295 (dccl_plane_angle))/s"
    std::string rewrite(const std::string& unit) const;

    ut_system* system() const { return system_; }
    const ut_unit* plane_angle() const { return plane_angle_; }
    const ut_unit* solid_angle() const { return solid_angle_; }
    const ut_unit* information() const { return information_; }

  private:
    ut_system* system_{nullptr};
    ut_unit* plane_angle_{nullptr};
    ut_unit* solid_angle_{nullptr};
    ut_unit* information_{nullptr};
};

} // namespace udunits
} // namespace units
} // namespace dccl

#endif
