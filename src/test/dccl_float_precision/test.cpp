// Copyright 2011-2026:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Cadmus To <cadmus.to+dev@gmail.com>
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

// Tests encoding and decoding of different float precisions on DefaultNumericFieldCodec

#include "../../codec.h"
#include "test.pb.h"

#include <array>
#include <cassert>
#include <cmath>

using namespace dccl::test;

// Issue #149: min/max limits of floating point fields are not always inclusive
// https://github.com/GobySoft/dccl/issues/149
//
// v4 codec fails (OutOfRangeException in strict mode) encoding a float equal to max boundary.
// v5 codec (PR #152 fix) succeeds for the same value.
static void test_issue149()
{
    dccl::Codec codec;

    // ---- v4 codec: encoding value == max should fail (demonstrates the bug) ----
    codec.load<Issue149V4Msg>();
    codec.set_strict(true);

    bool v4_threw = false;
    {
        Issue149V4Msg msg_in;
        msg_in.set_a(0.05f);
        try
        {
            std::string encoded;
            codec.encode(&encoded, msg_in);
        }
        catch (dccl::OutOfRangeException&)
        {
            v4_threw = true;
        }
    }
    assert(v4_threw && "issue #149: v4 codec must throw OutOfRangeException for value == max");
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "issue #149: v4 encoding of max boundary value threw OutOfRangeException as expected" << std::endl;

    codec.set_strict(false);

    // ---- v5 codec: encoding value == max must succeed (verifies the fix) ----
    codec.load<Issue149V5Msg>();
    codec.set_strict(true);

    {
        const float test_val = 0.05f;
        const float resolution = 0.01f; // precision=2 -> resolution=10^-2

        Issue149V5Msg msg_in;
        msg_in.set_a(test_val);

        std::string encoded;
        codec.encode(&encoded, msg_in);

        Issue149V5Msg msg_out;
        codec.decode(encoded, &msg_out);

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "issue #149: v5 encoding of max boundary value succeeded, decoded = "
                  << msg_out.a() << std::endl;
        assert(std::abs(msg_out.a() - test_val) <= resolution / 2 &&
               "issue #149: v5 decoded value too far from input for max boundary");
    }
}

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    dccl::Codec codec;

    // Simple testing
    codec.load<FloatMsg>();
    codec.info<FloatMsg>(&dccl::dlog);

    {
        const auto *field_ptr = FloatMsg::GetDescriptor()->FindFieldByName("f");
        assert(field_ptr);
        const auto &field = *field_ptr;

        const auto &opts = field.options();
        const auto &dccl_ext = opts.GetExtension(dccl::field);

        auto res = std::numeric_limits<double>::quiet_NaN();
        if (dccl_ext.has_precision()) {
            res = std::pow(10.0, -dccl_ext.precision());
        } else {
            res = dccl_ext.resolution();
        }

        const auto tol = res/2;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Tolerance for DCCL float = " << res << " / 2 = " << tol << std::endl;

        const auto test_cases = std::array<float, 3>{{0.000001f, 0.010001f, 1.000001f}};
        for (const auto test_case : test_cases) {
            auto msg_in = FloatMsg{};
            msg_in.set_f(test_case);

            auto encoded = std::string{};
            codec.encode(&encoded, msg_in);

            auto msg_out = FloatMsg{};
            codec.decode(encoded, &msg_out);

            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if encoded value " << msg_out.f() << " is close enough to " << test_case << "..." << std::endl;
            assert(std::abs(test_case - msg_out.f()) < tol);
        }
    }

    // Testing with negative precision
    codec.load<NegativePrecisionFloatMsg>();
    codec.info<NegativePrecisionFloatMsg>(&dccl::dlog);

    {
        const auto *field_ptr = NegativePrecisionFloatMsg::GetDescriptor()->FindFieldByName("f");
        assert(field_ptr);
        const auto &field = *field_ptr;

        const auto &opts = field.options();
        const auto &dccl_ext = opts.GetExtension(dccl::field);

        auto res = std::numeric_limits<double>::quiet_NaN();
        if (dccl_ext.has_precision()) {
            res = std::pow(10.0, -dccl_ext.precision());
        } else {
            res = dccl_ext.resolution();
        }

        const auto tol = res/2;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Tolerance for DCCL float = " << res << " / 2 = " << tol << std::endl;

        const auto test_cases = std::array<float, 3>{{10.f, -10.f, 1.f}};
        for (const auto test_case : test_cases) {
            auto msg_in = NegativePrecisionFloatMsg{};
            msg_in.set_f(test_case);

            auto encoded = std::string{};
            codec.encode(&encoded, msg_in);

            auto msg_out = NegativePrecisionFloatMsg{};
            codec.decode(encoded, &msg_out);

            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if encoded value " << msg_out.f() << " is close enough to " << test_case << "..." << std::endl;
            assert(std::abs(test_case - msg_out.f()) < tol);
        }
    }

    // Test conversion of values at different precisions
    codec.load<PrecisionRangeFloatMsg>();
    codec.info<PrecisionRangeFloatMsg>(&dccl::dlog);

    {
        auto resolutions = std::vector<double>{};
        resolutions.resize(7);
        for (auto field_idx = 0ul; field_idx < 7; ++field_idx) {
            const auto field_name = "prec" + std::to_string(field_idx);
            const auto *field_ptr = PrecisionRangeFloatMsg::GetDescriptor()->FindFieldByName(field_name);
            assert(field_ptr);
            const auto &field = *field_ptr;

            const auto &opts = field.options();
            const auto &dccl_ext = opts.GetExtension(dccl::field);

            if (dccl_ext.has_precision()) {
                resolutions[field_idx] = std::pow(10.0, -dccl_ext.precision());
            } else {
                resolutions[field_idx] = dccl_ext.resolution();
            }

            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Tolerance for DCCL float number " << field_idx << " is " << resolutions[field_idx] << " / 2 = " << resolutions[field_idx]/2 << std::endl;
        }

        for (auto i = -100; i < 101; ++i) {
            // Sweeping between the 6 and 7 decimal places of support for float.
            // Float may not be able to express some of these values, but that's not
            // in our scope. We're just making sure whatever float can express is
            // preserved on the other end.

            const auto test_val = 1000000+i;
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Testing encoding of " << test_val << " on a range of precisions..." << std::endl;
            auto msg_in = PrecisionRangeFloatMsg{};
            msg_in.set_prec0(test_val * resolutions[0]);
            msg_in.set_prec1(test_val * resolutions[1]);
            msg_in.set_prec2(test_val * resolutions[2]);
            msg_in.set_prec3(test_val * resolutions[3]);
            msg_in.set_prec4(test_val * resolutions[4]);
            msg_in.set_prec5(test_val * resolutions[5]);
            msg_in.set_prec6(test_val * resolutions[6]);

            auto encoded = std::string{};
            codec.encode(&encoded, msg_in);

            auto msg_out = PrecisionRangeFloatMsg{};
            codec.decode(encoded, &msg_out);

            const auto diff0 = std::abs(msg_out.prec0() - msg_in.prec0());
            const auto diff1 = std::abs(msg_out.prec1() - msg_in.prec1());
            const auto diff2 = std::abs(msg_out.prec2() - msg_in.prec2());
            const auto diff3 = std::abs(msg_out.prec3() - msg_in.prec3());
            const auto diff4 = std::abs(msg_out.prec4() - msg_in.prec4());
            const auto diff5 = std::abs(msg_out.prec5() - msg_in.prec5());
            const auto diff6 = std::abs(msg_out.prec6() - msg_in.prec6());
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec0() << " is close enough to " << msg_in.prec0() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff0 << "<=" << resolutions[0]/2 << std::endl;
            assert(diff0 <= resolutions[0]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec1() << " is close enough to " << msg_in.prec1() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff1 << "<=" << resolutions[1]/2 << std::endl;
            assert(diff1 <= resolutions[1]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec2() << " is close enough to " << msg_in.prec2() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff2 << "<=" << resolutions[2]/2 << std::endl;
            assert(diff2 <= resolutions[2]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec3() << " is close enough to " << msg_in.prec3() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff3 << "<=" << resolutions[3]/2 << std::endl;
            assert(diff3 <= resolutions[3]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec4() << " is close enough to " << msg_in.prec4() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff4 << "<=" << resolutions[4]/2 << std::endl;
            assert(diff4 <= resolutions[4]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec5() << " is close enough to " << msg_in.prec5() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff5 << "<=" << resolutions[5]/2 << std::endl;
            assert(diff5 <= resolutions[5]/2);
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Checking if " << msg_out.prec6() << " is close enough to " << msg_in.prec6() << "...";
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << " i.e. Difference " << diff6 << "<=" << resolutions[6]/2 << std::endl;
            assert(diff6 <= resolutions[6]/2);
        }
    }

    test_issue149();
    
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
}
