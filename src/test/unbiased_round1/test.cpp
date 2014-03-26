// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <cassert>
#include <utility>

#include "dccl/common.h"

bool same(double a, double b)
{
    return dccl::are_same(a,b);
}

bool same(int a, int b)
{
    return a == b;
}


template<typename T>
void check(T in, int prec, T out)
{
    std::cout << "Checking that " << in << " rounded to precision: " << prec << " is equal to " << out << std::endl;
    assert(same(dccl::unbiased_round(in, prec), out));
}

int main()
{
    check(1.234, 2, 1.23);
    check(1.25, 1, 1.2);
    check(1.35, 1, 1.4);
    
    check(1239, -1, 1240);
    check(1350, -2, 1400);
    check(1450, -2, 1400);
    check(1344, -3, 1000);
    
    check(1239.0, -1, 1240.0);
    check(1350.0, -2, 1400.0);
    check(1450.0, -2, 1400.0);
    check(1344.0, -3, 1000.0);
    
    check(-499000, -3, -499000);
    check(-500000, -3, -500000);

    check(-500000.0, -3, -500000.0);

    check(0, -3, 0);
    check(0, 2, 0);

    check(0.0, -3, 0.0);
    check(0.0, 2, 0.0);
    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
