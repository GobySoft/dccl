// Copyright 2024:
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
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>

#include "../../binary.h"

int main()
{
    // Basic encode/decode round-trip
    {
        std::string original = "Hello, World!";
        std::string encoded = dccl::b64_encode(original);
        std::string decoded = dccl::b64_decode(encoded);
        std::cout << "original: " << original << std::endl;
        std::cout << "encoded:  " << encoded << std::endl;
        std::cout << "decoded:  " << decoded << std::endl;
        assert(encoded == "SGVsbG8sIFdvcmxkIQ==");
        assert(decoded == original);
    }

    // Binary data round-trip
    {
        std::string binary_data("\x00\x01\x02\x03\xff\xfe\xfd", 7);
        std::string encoded = dccl::b64_encode(binary_data);
        std::string decoded = dccl::b64_decode(encoded);
        assert(decoded == binary_data);
    }

    // Empty string
    {
        std::string empty;
        std::string encoded = dccl::b64_encode(empty);
        std::string decoded = dccl::b64_decode(encoded);
        assert(encoded.empty());
        assert(decoded.empty());
    }

    // Known base64 values
    {
        assert(dccl::b64_encode("Man") == "TWFu");
        assert(dccl::b64_encode("Ma") == "TWE=");
        assert(dccl::b64_encode("M") == "TQ==");
        assert(dccl::b64_decode("TWFu") == "Man");
        assert(dccl::b64_decode("TWE=") == "Ma");
        assert(dccl::b64_decode("TQ==") == "M");
    }

    // Long string round-trip (1000 bytes)
    {
        std::string long_data(1000, '\0');
        std::iota(long_data.begin(), long_data.end(), 0); // fill with 0,1,2,...,231,232,...,255,0,1,...
        std::string encoded = dccl::b64_encode(long_data);
        std::string decoded = dccl::b64_decode(encoded);
        // encoded length must be a multiple of 4
        assert(encoded.size() % 4 == 0);
        assert(decoded == long_data);
    }

    std::cout << "all tests passed" << std::endl;

    return 0;
}
