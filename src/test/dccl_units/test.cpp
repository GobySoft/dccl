// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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
#include <iostream>
#include <iomanip>

#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/base_units/metric/bar.hpp>
#include <boost/units/physical_dimensions/pressure.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/temperature/celsius.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/dimensionless.hpp>
#include "test.pb.h"
#include "auv_status.pb.h"

#include <boost/units/base_units/metric/nautical_mile.hpp>

int main()
{
    CTDTestMessage test_msg;

    using namespace boost::units;
    using boost::units::metric::bar_base_unit;
    using boost::units::si::deci;
    
    typedef bar_base_unit::unit_type Bar;
    static const Bar bar;
    
    
    quantity<Bar> pressure(150.123456789*si::deci*bar);

    test_msg.set_pressure_with_units(pressure);

    typedef boost::units::unit<boost::units::temperature_dimension,boost::units::si::system> Kelvin;
    quantity<absolute<Kelvin> > temp(15*absolute<celsius::temperature>());
    std::cout << temp << std::endl;
    
    double temp_d = (temp - quantity<absolute<Kelvin> >(0*absolute<Kelvin>()))/Kelvin();
    std::cout << temp_d << std::endl;

    test_msg.set_temperature_with_units(15*absolute<fahrenheit::temperature>());
    test_msg.set_salinity(35.2);
    test_msg.set_sound_speed(1500);

    quantity<si::velocity> c(1500*si::meters_per_second);
    test_msg.set_sound_speed_with_units(c);    
    test_msg.set_depth_with_units(100*si::meters);
    quantity<si::velocity> auv_spd(2.5*si::meters_per_second);
    test_msg.set_auv_speed_with_units(auv_spd);
    std::cout <<"auv_spd: " <<auv_spd <<std::endl;
    
    test_msg.set_salinity_with_units(38.9*si::dimensionless());

    test_msg.mutable_no_units()->set_foo(10);
    
    std::cout << test_msg.DebugString() << std::endl; //outputs protobuf debug string
    std::cout << test_msg.DebugStringWithUnits() << std::endl;
    std::cout << "Temperature: " << test_msg.temperature_with_units() << std::endl;
    std::cout <<std::setprecision(10) << "Pressure: " << test_msg.pressure_with_units() << std::endl;
    std::cout << "Pressure (as bars): " << quantity<Bar>(test_msg.pressure_with_units()) << std::endl;
    std::cout << "Sound speed: " << test_msg.sound_speed_with_units() << std::endl;
    std::cout << "AUV speed: " << test_msg.auv_speed_with_units() << std::endl;
    std::cout << "Salinity: " << test_msg.salinity_with_units() << std::endl;

    AUVStatus status;
    status.set_x_with_units(1000*si::meters);
    status.set_y_with_units(500*si::meters);
    status.set_heading_with_units(3.1415926535/2*si::radians);
    
    typedef metric::nautical_mile_base_unit::unit_type NauticalMile;
    quantity<NauticalMile> x_nm(status.x_with_units());
    quantity<NauticalMile> y_nm(status.y_with_units());
    
    std::cout << status.DebugString() << std::endl;
    std::cout << status.DebugStringWithUnits() << std::endl;
    std::cout << x_nm << std::endl;
    std::cout << y_nm << std::endl;
    std::cout << status.heading_with_units() << std::endl;


    Parent p;
    p.set_mass_with_units(2*si::kilograms);
    p.set_si_mass_with_units(10*si::kilograms);
    p.mutable_child()->set_length_with_units(5*si::meters);
    
    assert(p.mass() == 2000); // grams
    assert(p.si_mass() == 10); // kilograms
    assert(p.child().length() == 500); // centimeters

    
    
    std::cout << "all tests passed" << std::endl;
}
