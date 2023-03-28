// Copyright 2013-2017:
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
#include <cassert>
#include <iostream>
#include <utility>

#include "../../binary.h"
#include "../../bitset.h"

using dccl::Bitset;

int main()
{
    // construct
    unsigned long value = 23;
    Bitset bits(8, value);
    std::string s = bits.to_string();
    std::cout << bits << std::endl;
    assert(s == std::string("00010111"));

    // bitshift
    bits <<= 2;
    std::cout << bits << std::endl;
    s = bits.to_string();
    assert(s == std::string("01011100"));

    bits >>= 1;
    std::cout << bits << std::endl;
    s = bits.to_string();
    assert(s == std::string("00101110"));

    s = (bits << 3).to_string();
    assert(s == std::string("01110000"));

    s = (bits >> 2).to_string();
    assert(s == std::string("00001011"));

    // logic
    unsigned long v1 = 14, v2 = 679;
    Bitset bits1(15, v1), bits2(15, v2);

    assert((bits1 & bits2).to_ulong() == (v1 & v2));
    assert((bits1 | bits2).to_ulong() == (v1 | v2));
    assert((bits1 ^ bits2).to_ulong() == (v1 ^ v2));

    assert((bits < bits1) == (value < v1));
    assert((bits1 < bits2) == (v1 < v2));
    assert((bits2 < bits1) == (v2 < v1));

    using namespace std::rel_ops;

    assert((bits1 > bits2) == (v1 > v2));
    assert((bits1 >= bits2) == (v1 >= v2));
    assert((bits1 <= bits2) == (v1 <= v2));
    assert((bits1 != bits2) == (v1 != v2));

    assert((Bitset(8, 23)) == Bitset(8, 23));
    assert((Bitset(16, 23)) != Bitset(8, 23));
    assert((Bitset(16, 0x0001)) != Bitset(16, 0x1001));

    assert(dccl::hex_encode(bits2.to_byte_string()) == "a702");
    bits2.from_byte_string(dccl::hex_decode("12a502"));
    std::cout << bits2.size() << ": " << bits2 << std::endl;
    assert(bits2.to_ulong() == 0x02a512);

    // get_more_bits;
    {
        std::cout << std::endl;
        Bitset parent(8, 0xD1);
        Bitset child(4, 0, &parent);

        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" << std::endl;
        child.get_more_bits(4);

        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 4);

        assert(child.to_ulong() == 0x10);
        assert(parent.to_ulong() == 0xD);
    }

    {
        std::cout << std::endl;
        Bitset grandparent(8, 0xD1);
        Bitset parent(8, 0x02, &grandparent);
        Bitset child(4, 0, &parent);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" << std::endl;
        child.get_more_bits(4);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 4);
        assert(grandparent.size() == 8);

        assert(child.to_ulong() == 0x20);
        assert(parent.to_ulong() == 0x0);
        assert(grandparent.to_ulong() == 0xD1);
    }

    {
        std::cout << std::endl;
        Bitset grandparent(8, 0xD1);
        Bitset parent(&grandparent);
        Bitset child(4, 0, &parent);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        std::cout << "get more bits: 4" << std::endl;
        child.get_more_bits(4);

        std::cout << "grandparent: " << grandparent << std::endl;
        std::cout << "parent: " << parent << std::endl;
        std::cout << "child: " << child << std::endl;

        assert(child.size() == 8);
        assert(parent.size() == 0);
        assert(grandparent.size() == 4);

        assert(child.to_ulong() == 0x10);
        assert(parent.to_ulong() == 0x0);
        assert(grandparent.to_ulong() == 0xD);
    }

    std::cout << "all tests passed" << std::endl;

    return 0;
}
