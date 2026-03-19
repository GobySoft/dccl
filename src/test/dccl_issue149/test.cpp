// Copyright 2025:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
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

// Unit test for GitHub issue #149:
//   "min max limits of floating point fields are not always inclusive"
//   https://github.com/GobySoft/dccl/issues/149
//
// Verifies that:
//  1. The v4 codec fails (throws OutOfRangeException in strict mode) when encoding a float
//     field value equal to the declared maximum (e.g. a = max = 0.05 with precision=2).
//  2. The v5 codec (as patched by PR #152) succeeds for the same field/value combination.

#include <cmath>

#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    // boundary values from the issue report
    // fails e.g. for 0.05, 0.07, 0.09, 0.10, 0.14, 0.15, 0.17, 0.18, 0.20
    const float boundary_values[] = {0.05f, 0.07f, 0.09f, 0.10f, 0.14f, 0.15f, 0.17f, 0.18f, 0.20f};
    const int num_values = sizeof(boundary_values) / sizeof(boundary_values[0]);

    // ---- v4 codec test ----
    // In v4 encoding a float value equal to the max boundary fails due to floating-point
    // quantization errors. In strict mode this throws OutOfRangeException.
    codec.load<Issue149V4Msg>();
    codec.info<Issue149V4Msg>(&dccl::dlog);
    codec.set_strict(true);

    // Collect which values fail in v4
    int v4_failures = 0;
    for (int i = 0; i < num_values; ++i)
    {
        float val = boundary_values[i];

        // Only test values that are <= the declared max (0.05 is the max in the proto)
        if (val > 0.05f)
            break;

        Issue149V4Msg msg_in;
        msg_in.set_a(val);

        try
        {
            std::string encoded;
            codec.encode(&encoded, msg_in);
            // If we reach here, encoding succeeded (unexpected for v4 with the unfixed code).
            // v4 may succeed for some values - we just note which ones fail.
            std::cout << "v4: encoding " << val << " succeeded" << std::endl;
        }
        catch (dccl::OutOfRangeException& e)
        {
            std::cout << "v4: encoding " << val
                      << " failed as expected (issue #149): " << e.what() << std::endl;
            ++v4_failures;
        }
    }

    // At least the exact max value (0.05) should fail in v4 due to issue #149
    // (floating-point quantization makes 0.05 appear slightly > max after rounding)
    assert(v4_failures > 0 &&
           "issue #149: expected v4 codec to fail encoding value equal to max boundary");
    std::cout << "v4 failures (expected): " << v4_failures << std::endl;

    codec.set_strict(false);

    // ---- v5 codec test ----
    // PR #152 fixes the issue by performing quantisation arithmetic in the integer domain,
    // so encoding any value within [min, max] (inclusive) must succeed.
    codec.load<Issue149V5Msg>();
    codec.info<Issue149V5Msg>(&dccl::dlog);
    codec.set_strict(true);

    // Test all values reported in the issue (0.05 is the declared max).
    // Only the value 0.05 is at the boundary for this message.
    {
        const float test_val = 0.05f;
        const float resolution = 0.01f; // precision=2 -> resolution=10^-2

        Issue149V5Msg msg_in;
        msg_in.set_a(test_val);

        std::string encoded;
        codec.encode(&encoded, msg_in);

        Issue149V5Msg msg_out;
        codec.decode(encoded, &msg_out);

        std::cout << "v5: encoding " << test_val << " succeeded, decoded = " << msg_out.a()
                  << std::endl;

        // The decoded value should be within one resolution step of the input
        assert(std::abs(msg_out.a() - test_val) <= resolution / 2 &&
               "v5 codec: decoded value too far from input for max boundary");
    }

    // Test a sweep of values reported as failing in the issue.
    // These include 0.05 (the max) and various values up to 0.20 for a broader field.
    // We use a separate codec with a wider max for the broader sweep.
    // Here we just verify that encoding and round-trip work for the exact issue scenario.
    {
        const float boundary_vals_at_max[] = {0.00f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f};
        const int n = sizeof(boundary_vals_at_max) / sizeof(boundary_vals_at_max[0]);
        const float resolution = 0.01f;

        for (int i = 0; i < n; ++i)
        {
            float val = boundary_vals_at_max[i];

            Issue149V5Msg msg_in;
            msg_in.set_a(val);

            std::string encoded;
            codec.encode(&encoded, msg_in);

            Issue149V5Msg msg_out;
            codec.decode(encoded, &msg_out);

            std::cout << "v5 sweep: in=" << val << " out=" << msg_out.a() << std::endl;
            assert(std::abs(msg_out.a() - val) <= resolution / 2 &&
                   "v5 codec: decoded value too far from input");
        }
    }

    std::cout << "all tests passed" << std::endl;
    return 0;
}
