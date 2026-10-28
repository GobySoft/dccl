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

// Sets each field of UnitsSyntaxMsg through its generated _with_units mutator, using a unit that
// differs from the field's own where possible, and checks the stored value.

#include <cmath>
#include <iostream>
#include <string>

#include <boost/units/base_units/metric/knot.hpp>
#include <boost/units/systems/si/prefixes.hpp>

#include "dccl/test/pb_plugin/units_syntax.pb.h"

namespace
{
int failures = 0;

void check(const std::string& what, double actual, double expected)
{
    if (std::abs(actual - expected) > 1e-6 * std::max(1.0, std::abs(expected)))
    {
        std::cerr << "FAIL: " << what << ": expected " << expected << ", got " << actual
                  << std::endl;
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
    using namespace boost::units;
    using boost::units::si::meters;
    using boost::units::si::seconds;
    const double pi = std::acos(-1.0);
    const auto knot = metric::knot_base_unit::unit_type();
    const auto byte = information::byte_base_unit::unit_type();

    dccl::test::plugin::UnitsSyntaxMsg msg;

    msg.set_named_base_with_units(2 * knot);
    check("named base dimensions", msg.named_base(), 2 * 1852.0 / 3600);
    msg.set_suffixed_base_with_units(3 * si::square_meters);
    check("suffixed base dimension", msg.suffixed_base(), 3);
    msg.set_divided_with_units(4 * si::newtons / si::square_meters);
    check("divided derived dimensions", msg.divided(), 4);
    msg.set_multiplied_with_units(5 * si::square_meters);
    check("multiplied derived dimensions", msg.multiplied(), 5);
    msg.set_juxtaposed_with_units(6 * si::kilograms * meters);
    check("juxtaposed derived dimensions", msg.juxtaposed(), 6);

    msg.set_time_us_with_units(1.5 * seconds);
    check("T with a micro prefix", msg.time_us(), 1500000);
    msg.set_distance_with_units(1 * si::kilo * meters);
    check("L", msg.distance(), 1000);
    msg.set_speed_with_units(2 * knot);
    check("LT^-1", msg.speed(), 2 * 1852.0 / 3600);
    msg.set_acceleration_with_units(9.81 * si::meters_per_second_squared);
    check("LT^-2", msg.acceleration(), 9.81);
    msg.set_frequency_with_units(2 * si::kilo * si::hertz);
    check("T^-1", msg.frequency(), 2000);
    msg.set_force_with_units(5 * si::newtons);
    check("MLT^-2", msg.force(), 5);
    msg.set_density_with_units(1025 * si::kilograms_per_cubic_meter);
    check("ML^-3", msg.density(), 1025);
    msg.set_area_with_units(1 * si::square_meters);
    check("L^2", msg.area(), 1);
    msg.set_charge_with_units(2 * si::coulombs);
    check("IT", msg.charge(), 2);
    msg.set_amount_with_units(3 * si::moles);
    check("N", msg.amount(), 3);
    msg.set_luminous_intensity_with_units(4 * si::candelas);
    check("J", msg.luminous_intensity(), 4);
    msg.set_solid_angle_with_units(5 * si::steradians);
    check("S", msg.solid_angle(), 5);
    msg.set_ratio_with_units(0.5 * si::dimensionless());
    check("-", msg.ratio(), 0.5);
    msg.set_heading_with_units(pi / 2 * si::radians);
    check("A in degrees", msg.heading(), 90);
    msg.set_turn_rate_with_units(180 * degree::degrees / seconds);
    check("AT^-1", msg.turn_rate(), pi);
    msg.set_water_temperature_with_units(300 * absolute<si::temperature>());
    check("K in celsius", msg.water_temperature(), 300 - 273.15);
    msg.set_mixed_with_units(36000 * meters / (3600 * seconds));
    check("abbreviated and named base dimensions", msg.mixed(), 10);

    msg.set_timestamp_us_with_units(2 * seconds);
    check("time with a micro prefix", msg.timestamp_us(), 2000000);
    msg.set_duration_ms_with_units(1.5 * seconds);
    check("time with a milli prefix", msg.duration_ms(), 1500);
    msg.set_vertical_speed_with_units(1 * knot);
    check("length/time", msg.vertical_speed(), 1852.0 / 3600);
    msg.set_linear_acceleration_with_units(2 * si::meters_per_second_squared);
    check("acceleration", msg.linear_acceleration(), 2);
    msg.set_sample_rate_with_units(10 * si::hertz);
    check("frequency", msg.sample_rate(), 10);
    msg.set_angular_velocity_with_units(90 * degree::degrees / seconds);
    check("angular_velocity", msg.angular_velocity(), pi / 2);
    msg.set_salinity_with_units(35 * si::dimensionless());
    check("dimensionless", msg.salinity(), 35);
    msg.set_conductivity_with_units(4 * si::siemens / meters);
    check("conductivity", msg.conductivity(), 4);
    msg.set_course_with_units(pi * si::radians);
    check("plane_angle in degrees", msg.course(), 180);
    msg.set_temperature_with_units(283.15 * absolute<si::temperature>());
    check("temperature in celsius", msg.temperature(), 10);

    msg.set_diffusivity_with_units(2 * si::square_meters / seconds);
    check("three derived dimensions", msg.diffusivity(), 2);
    msg.set_power_with_units(3 * si::watts);
    check("three spaced derived dimensions", msg.power(), 3);

    msg.set_pressure_dbar_with_units(101325 * si::pascals);
    check("metric::bar with a deci prefix", msg.pressure_dbar(), 10.1325);
    msg.set_duration_min_with_units(120 * seconds);
    check("metric::minute", msg.duration_min(), 2);
    msg.set_storage_kb_with_units(3000 * byte);
    check("information::byte with a kilo prefix", msg.storage_kb(), 3);

    // and back out, in a different unit from the field's
    check("reading LT^-1 in knots",
          msg.speed_with_units<quantity<metric::knot_base_unit::unit_type>>().value(), 2);
    check("reading degrees in radians", msg.heading_with_units<quantity<si::plane_angle>>().value(),
          pi / 2);

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
