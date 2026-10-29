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

// Uses (dccl.field).udunits metadata through the generated Boost Units accessors: sets an
// AUVStatus with Boost Units quantities, encodes and decodes it with DCCL, and reads the
// decoded values back in other units.

#include <cmath>
#include <iostream>
#include <string>

#include <boost/units/base_units/imperial/foot.hpp>
#include <boost/units/base_units/metric/knot.hpp>
#include <boost/units/base_units/metric/nautical_mile.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/information.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>

#include "dccl/codec.h"
#include "dccl/test/dccl_udunits_boost/auv_status.pb.h"

namespace
{
int failures = 0;

void check(const std::string& what, double actual, double expected, double tolerance)
{
    if (std::abs(actual - expected) > tolerance)
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
    using Knot = metric::knot_base_unit::unit_type;
    using NauticalMile = metric::nautical_mile_base_unit::unit_type;
    using Foot = imperial::foot_base_unit::unit_type;
    const double pi = std::acos(-1.0);

    dccl::test::udunits::AUVStatus status;
    status.set_timestamp(1.7e9);
    status.set_source(1);
    status.set_destination(2);

    // set each field in a unit other than its own where possible
    status.set_x_with_units(0.5 * NauticalMile());
    status.set_y_with_units(-1.5 * si::kilo * meters);
    status.set_speed_with_units(1.5 * si::meters_per_second);
    status.set_heading_with_units(pi / 2 * si::radians);
    status.set_depth_with_units(100 * meters);
    status.set_pitch_with_units(-10 * degree::degrees);
    status.set_turn_rate_with_units(0.2 * si::radians_per_second);
    status.set_water_temperature_with_units(50 * absolute<fahrenheit::temperature>());
    status.set_salinity_with_units(35.2 * si::dimensionless());
    status.set_latitude_with_units(41.5 * degree::degrees);
    status.set_longitude_with_units(-70.67 * degree::degrees);
    status.set_modem_rate_with_units(640 * information::bytes / seconds);
    status.add_sonar_ranges_with_units(10 * Foot());
    status.add_sonar_ranges_with_units(25 * meters);

    // the stored values are in each field's own units
    check("x stored in m", status.x(), 926, 1e-9);
    check("speed stored in knots", status.speed(), 1.5 * 3600 / 1852, 1e-9);
    check("heading stored in degrees", status.heading(), 90, 1e-9);
    check("depth stored in whole feet", status.depth(), 328, 0);
    check("pitch stored in mrad", status.pitch(), -10 * pi / 180 * 1000, 1e-9);
    check("water temperature stored in degC", status.water_temperature(), 10, 1e-9);
    check("modem rate stored in bit/s", status.modem_rate(), 5120, 1e-9);

    dccl::Codec codec;
    codec.load<dccl::test::udunits::AUVStatus>();
    std::string encoded;
    codec.encode(&encoded, status);
    std::cout << "encoded to " << encoded.size() << " bytes" << std::endl;

    dccl::test::udunits::AUVStatus decoded;
    codec.decode(encoded, &decoded);

    // read back through the Boost Units accessors, in other units, to within each field's
    // DCCL resolution
    check("x in nautical miles", quantity<NauticalMile>(decoded.x_with_units()).value(), 0.5,
          0.5 / 1852);
    check("y in km", decoded.y_with_units().value() / 1000, -1.5, 0.5e-3);
    check("speed in m/s", quantity<si::velocity>(decoded.speed_with_units()).value(), 1.5,
          0.05 * 1852 / 3600);
    check("speed in knots", decoded.speed_with_units<quantity<Knot>>().value(), 1.5 * 3600 / 1852,
          0.05);
    check("heading in radians", quantity<si::plane_angle>(decoded.heading_with_units()).value(),
          pi / 2, 0.05 * pi / 180);
    check("heading in degrees", decoded.heading_with_units<quantity<degree::plane_angle>>().value(),
          90, 0.05);
    check("depth in m", decoded.depth_with_units().value(), 328 * 0.3048, 1e-9);
    check("pitch in degrees", decoded.pitch_with_units<quantity<degree::plane_angle>>().value(),
          -10, 0.5e-3 * 180 / pi);
    check("turn rate in rad/s", decoded.turn_rate_with_units().value(), 0.2, 0.05 * pi / 180);
    check(
        "water temperature in degF",
        quantity<absolute<fahrenheit::temperature>>(decoded.water_temperature_with_units()).value(),
        50, 0.005 * 1.8);
    check("water temperature in K", decoded.water_temperature_with_units().value(), 283.15, 0.005);
    check("practical salinity", decoded.salinity_with_units().value(), 35.2, 0.005);
    check("latitude in degrees",
          decoded.latitude_with_units<quantity<degree::plane_angle>>().value(), 41.5, 0.5e-6);
    check("longitude in degrees",
          decoded.longitude_with_units<quantity<degree::plane_angle>>().value(), -70.67, 0.5e-6);
    check("modem rate in bytes/s",
          decoded
              .modem_rate_with_units<
                  quantity<divide_typeof_helper<information::hu::byte::info, si::time>::type>>()
              .value(),
          640, 0.5 / 8);
    check("first sonar range in ft", decoded.sonar_ranges_with_units<quantity<Foot>>(0).value(), 10,
          0.05 / 0.3048);
    check("second sonar range in m", decoded.sonar_ranges_with_units(1).value(), 25, 0.05);

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
