// Copyright 2015-2023:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Stephanie Petillo <stephanie@gobysoft.org>
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
#include <iomanip>
#include <iostream>

#include "dccl/logger.h"

#include "auv_status.pb.h"
#include "test.pb.h"
#include <boost/units/base_units/metric/bar.hpp>
#include <boost/units/io.hpp>
#include <boost/units/physical_dimensions/pressure.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/dimensionless.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/temperature/celsius.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>

#include <boost/units/base_units/metric/nautical_mile.hpp>

#include "dccl/units/conductivity.h"

int main()
{
    CTDTestMessage test_msg;

    using namespace boost::units;
    using boost::units::metric::bar_base_unit;
    using boost::units::si::deci;

    typedef bar_base_unit::unit_type Bar;
    static const Bar bar;

    quantity<Bar> pressure(150.123456789 * si::deci * bar);

    test_msg.set_pressure_with_units(pressure);

    using Kelvin =
        boost::units::unit<boost::units::temperature_dimension, boost::units::si::system>;
    quantity<absolute<Kelvin>> temp(15 * absolute<celsius::temperature>());
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << temp << std::endl;

    double temp_d = (temp - quantity<absolute<Kelvin>>(0 * absolute<Kelvin>())) / Kelvin();
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << temp_d << std::endl;

    test_msg.set_temperature_with_units(15 * absolute<fahrenheit::temperature>());
    test_msg.set_micro_temp_with_units(15 * Kelvin());
    test_msg.set_salinity(35.2);
    test_msg.set_sound_speed(1500);

    quantity<si::velocity> c(1500 * si::meters_per_second);
    test_msg.set_sound_speed_with_units(c);
    test_msg.set_depth_with_units(100 * si::meters);
    quantity<si::velocity> auv_spd(2.5 * si::meters_per_second);
    test_msg.set_auv_speed_with_units(auv_spd);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "auv_spd: " << auv_spd << std::endl;

    test_msg.set_conductivity_with_units(45.0 * dccl::units::siemens_per_m);

    test_msg.set_salinity_with_units(38.9 * si::dimensionless());

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << test_msg.DebugString() << std::endl; //outputs protobuf debug string
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Temperature: " << test_msg.temperature_with_units() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Micro temperature: " << test_msg.micro_temp_with_units() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << std::setprecision(10) << "Pressure: " << test_msg.pressure_with_units()
              << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Pressure (as bars): " << quantity<Bar>(test_msg.pressure_with_units())
              << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Sound speed: " << test_msg.sound_speed_with_units() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "AUV speed: " << test_msg.auv_speed_with_units() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Salinity: " << test_msg.salinity_with_units() << std::endl;

    assert(test_msg.conductivity() == 450000); // uS/cm

    AUVStatus status;
    status.set_x_with_units(1000 * si::meters);
    status.set_y_with_units(500 * si::meters);
    status.set_heading_with_units(3.1415926535 / 2 * si::radians);
    status.set_heading_rate_with_units(10 * boost::units::degree::degrees /
                                       boost::units::si::seconds);

    // Test angular velocity: set in rad/s (SI), read back as rad/s
    status.set_angular_velocity_with_units(1.0 * si::radians_per_second);
    assert(std::abs(status.angular_velocity() - 1.0) < 1e-3);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Angular velocity: " << status.angular_velocity_with_units() << std::endl;

    using NauticalMile = metric::nautical_mile_base_unit::unit_type;
    quantity<NauticalMile> x_nm(status.x_with_units());
    quantity<NauticalMile> y_nm(status.y_with_units());

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << status.DebugString() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << x_nm << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << y_nm << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << status.heading_with_units() << std::endl;
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << status.heading_rate_with_units() << std::endl;

    assert(status.heading_rate() > 9.9999 && status.heading_rate() < 10.0001); 

    Parent p;
    p.set_mass_with_units(2 * si::kilograms);
    p.set_si_mass_with_units(10 * si::kilograms);
    p.mutable_child()->set_length_with_units(5 * si::meters);

    assert(p.mass() == 2000);          // grams
    assert(p.si_mass() == 10);         // kilograms
    assert(p.child().length() == 500); // centimeters

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
}
