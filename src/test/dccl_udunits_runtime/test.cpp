// Copyright 2026:
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

// Tests run-time conversion with dccl/units/udunits.h, directly and through the generated
// <field>_with_udunits() accessors.

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "dccl/codec.h"
#include "dccl/test/dccl_udunits_runtime/runtime.pb.h"
#include "dccl/units/udunits.h"

namespace
{
int failures = 0;

void check(const std::string& what, bool ok)
{
    if (ok)
    {
        std::cout << "ok: " << what << std::endl;
    }
    else
    {
        std::cerr << "FAIL: " << what << std::endl;
        ++failures;
    }
}

void check_close(const std::string& what, double actual, double expected)
{
    const bool ok = std::abs(actual - expected) <= 1e-6 * std::max(1.0, std::abs(expected));
    check(what + " (expected " + std::to_string(expected) + ", got " + std::to_string(actual) + ")",
          ok);
}

void check_throws(const std::string& what, const std::function<void()>& f,
                  const std::string& expected)
{
    try
    {
        f();
        check(what + " throws", false);
    }
    catch (const dccl::Exception& e)
    {
        check(what + " throws \"" + expected + "\" (got \"" + e.what() + "\")",
              std::string(e.what()).find(expected) != std::string::npos);
    }
}
} // namespace

int main()
{
    namespace udunits = dccl::units::udunits;
    using dccl::test::udunits::RuntimeMsg;
    const double pi = std::acos(-1.0);

    // conversion
    check_close("knot to m/s", udunits::convert(1, "knot", "m/s"), 1852.0 / 3600);
    check_close("degC to degF", udunits::convert(100, "degC", "degF"), 212);
    check_close("degree/s to rpm", udunits::convert(360, "degree/s", "rpm"), 60);
    check_close("kilobyte to bit", udunits::convert(1, "kilobyte", "bit"), 8000);
    check("knot converts to m/s", udunits::convertible("knot", "m/s"));
    check("degree doesn't convert to percent", !udunits::convertible("degree", "percent"));
    check("byte/s doesn't convert to Hz", !udunits::convertible("byte/s", "Hz"));
    check_throws(
        "converting m to s", [] { udunits::convert(1, "m", "s"); },
        "\"m\" can't be converted to \"s\"");
    check_throws(
        "an unparseable unit", [] { udunits::convert(1, "deg", "rad"); },
        "\"deg\" is not a unit that UDUNITS-2 can parse");
    check_throws("ppt", [] { udunits::convertible("ppt", "1"); }, "parts per trillion");

    // the generated accessors
    RuntimeMsg msg;
    msg.set_speed_with_udunits(1, "m/s");
    check_close("set speed in m/s, stored in knots", msg.speed(), 3600 / 1852.0);
    check_close("speed read in km/h", msg.speed_with_udunits("km/h"), 3.6);
    msg.set_depth_with_udunits(100, "m");
    check("set an integer field rounds (100 m = 328.08 ft)", msg.depth() == 328);
    check_close("depth read in m", msg.depth_with_udunits("m"), 328 * 0.3048);
    msg.set_heading_with_udunits(pi / 2, "rad");
    check_close("set a float field", msg.heading(), 90);
    msg.set_water_temperature_with_udunits(50, "degF");
    check_close("an offset unit", msg.water_temperature(), 10);
    check_close("an offset unit read in K", msg.water_temperature_with_udunits("K"), 283.15);
    msg.add_sonar_ranges_with_udunits(10, "ft");
    msg.add_sonar_ranges_with_udunits(25, "m");
    msg.set_sonar_ranges_with_udunits(1, 30, "m");
    check_close("add to a repeated field", msg.sonar_ranges(0), 3.048);
    check_close("set an index of a repeated field", msg.sonar_ranges(1), 30);
    check_close("read an index of a repeated field", msg.sonar_ranges_with_udunits(0, "ft"), 10);
    msg.set_storage_with_udunits(3e6, "bit");
    check("an unsigned integer field", msg.storage() == 375);
    check("the unit string is generated too", std::string(RuntimeMsg::speed_udunits()) == "knot");
    check_throws(
        "setting from an incompatible unit", [&] { msg.set_speed_with_udunits(1, "s"); },
        "can't be converted");

    // reflection
    const auto* desc = RuntimeMsg::descriptor();
    const auto* speed = desc->FindFieldByName("speed");
    const auto* sonar_ranges = desc->FindFieldByName("sonar_ranges");
    check("unit_of a field with udunits", udunits::unit_of(speed) == "knot");
    check("unit_of a field without", udunits::unit_of(desc->FindFieldByName("plain")).empty());
    check_close("get through reflection", udunits::get(msg, speed, "km/h"), 3.6);
    udunits::set(&msg, speed, 2, "m/s");
    check_close("set through reflection", msg.speed(), 2 * 3600 / 1852.0);
    udunits::set(&msg, sonar_ranges, 50, "m");
    check("set with index -1 adds", msg.sonar_ranges_size() == 3 && msg.sonar_ranges(2) == 50);
    check_close("get a repeated field through reflection", udunits::get(msg, sonar_ranges, "cm", 1),
                3000);
    check_throws(
        "a field without udunits", [&] { udunits::get(msg, desc->FindFieldByName("plain"), "m"); },
        "has no (dccl.field).udunits");
    check_throws(
        "an index out of range", [&] { udunits::get(msg, sonar_ranges, "m", 3); }, "out of range");

    // DCCL encodes the stored values, so converted values survive a round trip
    dccl::Codec codec;
    codec.load<RuntimeMsg>();
    RuntimeMsg to_send;
    to_send.set_speed_with_udunits(1.5, "m/s");
    to_send.set_water_temperature_with_udunits(283.15, "K");
    std::string encoded;
    codec.encode(&encoded, to_send);
    RuntimeMsg received;
    codec.decode(encoded, &received);
    check_close("speed after encoding, in m/s", std::round(received.speed_with_udunits("m/s") * 10),
                std::round(1.5 * 10));
    check_close("temperature after encoding, in K", received.water_temperature_with_udunits("K"),
                283.15);

#if DCCL_THREAD_SUPPORT
    // concurrent conversions share the parser and converter cache
    std::vector<std::thread> threads;
    std::vector<int> thread_failures(8, 0);
    for (int t = 0; t < 8; ++t)
    {
        threads.emplace_back(
            [t, &thread_failures]
            {
                const std::string to = (t % 2) ? "km/h" : "ft/s";
                const double expected = (t % 2) ? 1.852 : 1852.0 / 3600 / 0.3048;
                for (int i = 0; i < 200; ++i)
                {
                    if (std::abs(udunits::convert(1, "knot", to) - expected) > 1e-9)
                        ++thread_failures[t];
                }
            });
    }
    for (auto& thread : threads) thread.join();
    int total_thread_failures = 0;
    for (int f : thread_failures) total_thread_failures += f;
    check("concurrent conversions agree", total_thread_failures == 0);
#endif

    if (failures)
    {
        std::cerr << failures << " failure(s)" << std::endl;
        return 1;
    }
    std::cout << "all tests passed" << std::endl;
    return 0;
}
