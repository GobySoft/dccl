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

#ifndef DCCL_UNITS_UDUNITS_H
#define DCCL_UNITS_UDUNITS_H

#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "dccl/def.h"

#if !DCCL_HAS_UDUNITS_RUNTIME
#error "dccl/units/udunits.h needs DCCL built with -Denable_udunits_runtime=ON"
#endif

// Run-time unit conversion with UDUNITS-2, used by the <field>_with_udunits() accessors that
// protoc-gen-dccl generates for (dccl.field).udunits. Unit strings are UDUNITS-2 units with DCCL's
// special cases: plane angle, solid angle and information are dimensions of their own, so "degree"
// doesn't convert to "percent", and "ppt" is rejected as ambiguous.
//
// All functions are thread safe. They throw dccl::Exception for a unit that can't be parsed or a
// conversion between incompatible units.

namespace dccl
{
namespace units
{
namespace udunits
{
// Converts value from unit "from" to unit "to"
double convert(double value, const std::string& from, const std::string& to);

// Whether a value in unit "from" can be converted to unit "to"
bool convertible(const std::string& from, const std::string& to);

// The field's (dccl.field).udunits, or an empty string if it has none
std::string unit_of(const google::protobuf::FieldDescriptor* field);

// Gets a numeric field's value (or, for a repeated field, the value at index) in unit, converted
// from the field's (dccl.field).udunits
double get(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field,
           const std::string& unit, int index = -1);

// Sets a numeric field's value (or, for a repeated field, the value at index; index -1 adds a
// value) from value in unit, converted to the field's (dccl.field).udunits. Integer fields are
// rounded to the nearest value.
void set(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field,
         double value, const std::string& unit, int index = -1);
} // namespace udunits
} // namespace units
} // namespace dccl

#endif
