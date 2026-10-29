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

// Sets each field of UdunitsBoostMsg through the Boost Units mutator translated from its
// (dccl.field).udunits, and checks the stored value.

#include <cmath>
#include <iostream>
#include <string>

#include <boost/units/base_units/imperial/foot.hpp>
#include <boost/units/base_units/metric/knot.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/information.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>

#include "dccl/test/pb_plugin/udunits_boost.pb.h"

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
    const double knot_in_m_per_s = 1852.0 / 3600;
    const auto knot = metric::knot_base_unit::unit_type();

    dccl::test::plugin::UdunitsBoostMsg msg;

    msg.set_speed_with_units(1 * si::meters_per_second);
    check("knot", msg.speed(), 1 / knot_in_m_per_s);
    check("knot read back in knots",
          msg.speed_with_units<quantity<metric::knot_base_unit::unit_type>>().value(),
          1 / knot_in_m_per_s);
    msg.set_speed_with_units(3 * knot);
    check("knot set in knots", msg.speed(), 3);

    msg.set_distance_with_units(1500 * meters);
    check("km on an integer field rounds", msg.distance(), 2);
    msg.set_distance_with_units(1499 * meters);
    check("km on an integer field rounds down", msg.distance(), 1);
    check("km read back in m", msg.distance_with_units().value(), 1000);
    msg.set_whole_meters_with_units(2.7 * meters);
    check("m on an integer field rounds", msg.whole_meters(), 3);
    msg.set_whole_meters_with_units(-2.7 * meters);
    check("m on an integer field rounds a negative value", msg.whole_meters(), -3);
    check("m read back", msg.whole_meters_with_units().value(), -3);

    msg.set_water_temperature_with_units(300 * absolute<si::temperature>());
    check("degC", msg.water_temperature(), 300 - 273.15);
    check("degC read back in K", msg.water_temperature_with_units().value(), 300);
    msg.set_air_temperature_with_units(273.15 * absolute<si::temperature>());
    check("degF", msg.air_temperature(), 32);
    msg.set_air_temperature_with_units(212 * absolute<fahrenheit::temperature>());
    check("degF set in degF", msg.air_temperature(), 212);
    msg.set_temperature_change_with_units(5 * si::kelvin);
    check("K as a temperature difference", msg.temperature_change(), 5);
    msg.set_acceleration_with_units(9.81 * si::meters_per_second_squared);
    check("m s-2", msg.acceleration(), 9.81);
    msg.add_depths_with_units(1 * meters);
    msg.add_depths_with_units(2 * imperial::foot_base_unit::unit_type());
    msg.set_depths_with_units(0, 0.3048 * meters);
    check("repeated ft", msg.depths(0), 1);
    check("repeated ft added", msg.depths(1), 2);
    check("repeated ft read back in m", msg.depths_with_units(1).value(), 0.6096);
    msg.set_pressure_with_units(101325 * si::pascals);
    check("dbar on a float field", msg.pressure(), 10.1325);

    msg.set_heading_with_units(pi / 2 * si::radians);
    check("degree", msg.heading(), 90);
    check("degree read back in degrees",
          msg.heading_with_units<quantity<degree::plane_angle>>().value(), 90);
    msg.set_turn_rate_with_units(1 * si::radians_per_second);
    check("degree/s", msg.turn_rate(), 180 / pi);
    msg.set_propeller_speed_with_units(2 * pi * si::radians_per_second);
    check("rpm", msg.propeller_speed(), 60);
    msg.set_bearing_error_with_units(1 * degree::degrees);
    check("arc minute", msg.bearing_error(), 60);
    msg.set_pitch_with_units(0.5 * si::radians);
    check("mrad", msg.pitch(), 500);
    msg.set_latitude_with_units(pi / 4 * si::radians);
    check("degree_north", msg.latitude(), 45);
    msg.set_longitude_with_units(pi / 4 * si::radians);
    check("degree_east", msg.longitude(), 45);
    msg.set_west_longitude_with_units(pi / 4 * si::radians);
    check("degree_west stores east as negative", msg.west_longitude(), -45);
    msg.set_beam_width_with_units(2 * si::steradians);
    check("sr", msg.beam_width(), 2);
    msg.set_luminous_flux_with_units(3 * si::lumens);
    check("lm", msg.luminous_flux(), 3);
    msg.set_illuminance_with_units(4 * si::lux);
    check("lux", msg.illuminance(), 4);

    msg.set_data_rate_with_units(800 * information::bits / seconds);
    check("byte/s", msg.data_rate(), 100);
    msg.set_storage_with_units(3000 * information::bytes);
    check("kilobyte", msg.storage(), 3);
    msg.set_bits_with_units(2 * information::bytes);
    check("bit", msg.bits(), 16);

    msg.set_fraction_with_units(0.25 * si::dimensionless());
    check("percent", msg.fraction(), 25);
    msg.set_concentration_with_units(1e-6 * si::dimensionless());
    check("ppm", msg.concentration(), 1);
    msg.set_absolute_salinity_with_units(0.035 * si::dimensionless());
    check("g/kg", msg.absolute_salinity(), 35);
    msg.set_ratio_with_units(0.5 * si::dimensionless());
    check("1", msg.ratio(), 0.5);

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
