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

#ifndef DCCL_PB_PLUGIN_UDUNITS_GEN_H
#define DCCL_PB_PLUGIN_UDUNITS_GEN_H

#include <string>
#include <vector>

// UDUNITS-2 support for protoc-gen-dccl, compiled in only when enable_udunits is on

namespace dccl
{
namespace units
{
namespace udunits
{
// Throws std::runtime_error unless UDUNITS-2, with DCCL's special cases, can parse unit
void validate(const std::string& unit);

// The Boost Units equivalent of a UDUNITS-2 unit: its dimension in Boost Units base dimensions
// (as accepted by (dccl.field).units.base_dimensions), and the conversion of a value in the unit
// to the coherent SI unit of that dimension: si_value = scale * value + offset
struct BoostUnits
{
    std::vector<std::string> dimensions;
    std::vector<double> powers;
    double scale{1};
    double offset{0};
};

// Throws std::runtime_error if unit has no Boost Units equivalent (e.g. a timestamp)
BoostUnits to_boost_units(const std::string& unit);
} // namespace udunits
} // namespace units
} // namespace dccl

#endif
