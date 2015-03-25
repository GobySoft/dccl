// Copyright 2014-2015 Toby Schneider (https://launchpad.net/~tes)
//                     Stephanie Petillo (https://launchpad.net/~spetillo)
//                     GobySoft, LLC
//
// This file is part of the Dynamic Compact Control Language Applications
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>

#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/base_units/metric/bar.hpp>
#include <boost/units/physical_dimensions/pressure.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/temperature/celsius.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include "test.pb.h"

#include <boost/units/base_units/metric/nautical_mile.hpp>

int main()
{
    CTDTestMessage test_msg;

    using namespace boost::units;
    using boost::units::metric::bar_base_unit;
    using boost::units::si::deci;
    
    typedef bar_base_unit::unit_type Bar;
    static const Bar bar;
    
    
    quantity<Bar> pressure(150.0*si::deci*bar);

    test_msg.set_pressure_with_units(pressure);

    quantity<absolute<celsius::temperature> > temp(15*absolute<celsius::temperature>());

    std::cout << temp << std::endl;
    
    
//    test_msg.set_temperature_with_units();
    test_msg.set_salinity(35.2);
    test_msg.set_sound_speed(1500);

    quantity<si::velocity> c(1500*si::meters_per_second);
    test_msg.set_sound_speed_with_units(c);    
    test_msg.set_depth_with_units(100*si::meters);

    
    
    std::cout << test_msg.DebugString() << std::endl;
    std::cout << "Temperature: " << test_msg.temperature_with_units() << std::endl;
    std::cout << "Pressure: " << test_msg.pressure_with_units() << std::endl;
    std::cout << "Sound speed: " << test_msg.sound_speed_with_units() << std::endl;

    AUVStatus status;
    status.set_x_with_units(1000*si::meters);
    status.set_y_with_units(500*si::meters);

    typedef metric::nautical_mile_base_unit::unit_type NauticalMile;
    quantity<NauticalMile> x_nm(status.x_with_units());
    quantity<NauticalMile> y_nm(status.y_with_units());
    
    std::cout << status.DebugString() << std::endl;
    std::cout << x_nm << std::endl;
    std::cout << y_nm << std::endl;
    
    
    std::cout << "all tests passed" << std::endl;
}
