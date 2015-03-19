// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
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

#include "test.pb.h"



int main()
{
    CTDTestMessage test_msg;

    using namespace boost::units;
    using boost::units::metric::bar_base_unit;
    using boost::units::si::deci;
    
    
    static const bar_base_unit::unit_type bar;
    
    
    quantity<si::pressure> pressure(150.0*si::deci*bar);

    test_msg.set_pressure_with_units(pressure);

    quantity<absolute<celsius::temperature> > temp(15*absolute<celsius::temperature>());

    std::cout << temp << std::endl;
    
    
//    test_msg.set_temperature_with_units();
    test_msg.set_salinity(35.2);
    test_msg.set_sound_speed(1500);

    std::cout << test_msg.DebugString() << std::endl;
    std::cout << "Temperature: " << test_msg.temperature_with_units() << std::endl;
    std::cout << "Pressure: " << test_msg.pressure_with_units() << std::endl;
    std::cout << "Sound speed: " << test_msg.sound_speed_with_units() << std::endl;
    
    
    std::cout << "all tests passed" << std::endl;
}
