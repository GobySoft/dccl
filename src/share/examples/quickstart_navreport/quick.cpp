// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
#include "dccl.h"
#include "navreport.pb.h"
#include <iostream>

int main()
{
    std::string encoded_bytes;
    dccl::Codec codec;
    codec.load<NavigationReport>();
    // SENDER
    {
        NavigationReport r_out;
        r_out.set_x(450);
        r_out.set_y(550);
        r_out.set_z(-100);
        r_out.set_veh_class(NavigationReport::AUV);
        r_out.set_battery_ok(true);
        
        codec.encode(&encoded_bytes, r_out);
    }
    // send encoded_bytes across your link

    // RECEIVER
    if(codec.id(encoded_bytes) == codec.id<NavigationReport>())
    {
        NavigationReport r_in;
        codec.decode(encoded_bytes, &r_in);
        std::cout << r_in.ShortDebugString() << std::endl;
    }    
}
