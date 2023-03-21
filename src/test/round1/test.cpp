// Copyright 2011-2017:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
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
#include <iostream>
#include <cassert>
#include <utility>

#include "dccl/common.h"

bool same(double a, double b)
{
    // good enough comparison for this test
    return std::abs(a-b) < 1e-10;    
}

template<typename Int>
bool same(Int a, Int b)
{
    return a == b;
}


template<typename T>
void check(T in, int prec, T out)
{
    std::cout << "Checking that " << in << " rounded to precision: " << prec << " is equal to " << out << std::endl;
    assert(same(dccl::round(in, prec), out));
}

int main()
{
    check(1.234, 2, 1.23);
    check(1.25, 1, 1.3);
    check(1.35, 1, 1.4);
    
    check<int>(1239, -1, 1240);
    check<int>(1351, -2, 1400);
    check<int>(1450, -2, 1500);
    check<int>(1344, -3, 1000);
    
    check(1239.0, -1, 1240.0);
    check(1351.0, -2, 1400.0);
    check(1450.0, -2, 1500.0);
    check(1344.0, -3, 1000.0);
    
    check<int>(-499000, -3, -499000);
    check<int>(-500000, -3, -500000);

    check(-500000.0, -3, -500000.0);

    check<int>(0, -3, 0);
    check<int>(0, 2, 0);

    check(0.0, -3, 0.0);
    check(0.0, 2, 0.0);

    check<dccl::int64>(1409165969804999ull, -3, 1409165969805000ull);
    
    std::cout << "all tests passed" << std::endl;
    
    return 0;
}
