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

// Tests the CCL (Compact Control Language) scalar conversions in WhoiUtil.
// These define the legacy wire representation, so the encodings are pinned
// here rather than only round-tripped.

#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>

#include "dccl/logger.h"

#include "../../ccl/WhoiUtil.h"

namespace
{
bool close(double a, double b, double tol)
{
    bool ok = std::abs(a - b) <= tol;
    if (!ok)
        std::cerr << "expected " << b << " (+/- " << tol << "), got " << a << std::endl;
    return ok;
}

void check_latlon()
{
    // ~2 m resolution over the +/-180 degree range
    const double tol = 3e-5;
    for (double deg : {0.0, 1.0, -1.0, 42.35, -42.35, 179.9, -179.9, 89.999, -89.999})
        assert(close(Decode_latlon(Encode_latlon(deg)), deg, tol));

    // the sign extension on decode is the subtle part: negatives must not
    // come back as large positives
    assert(Decode_latlon(Encode_latlon(-120.0)) < 0);
    assert(Decode_latlon(Encode_latlon(120.0)) > 0);
    assert(close(Decode_latlon(Encode_latlon(0.0)), 0.0, 1e-9));
}

void check_heading()
{
    for (float deg : {0.0f, 45.0f, 90.0f, 180.0f, 270.0f, 359.0f})
        assert(close(Decode_heading(Encode_heading(deg)), deg, 1.5));

    assert(Encode_heading(0.0f) == 0);
    assert(Encode_heading(360.0f) == 255);
}

void check_est_velocity()
{
    // 2.5 cm/sec resolution
    for (float v : {0.0f, 0.5f, 1.0f, 2.5f, 5.0f})
        assert(close(Decode_est_velocity(Encode_est_velocity(v)), v, 0.03));

    // the +0.5 in the encoder only rounds positives, so negatives truncate
    // toward zero and lose up to a full step
    for (float v : {-1.0f, -2.5f, -5.0f})
        assert(close(Decode_est_velocity(Encode_est_velocity(v)), v, 0.05));
    assert(Encode_est_velocity(-1.0f) == -24);

    assert(Encode_est_velocity(0.0f) == 0);
}

void check_salinity()
{
    // valid range is 20-45 ppt; anything below 20 encodes to 0
    assert(Encode_salinity(19.9f) == 0);
    assert(Encode_salinity(0.0f) == 0);
    assert(Decode_salinity(0) == 0.0f);

    for (float s : {20.1f, 25.0f, 33.5f, 45.0f})
        assert(close(Decode_salinity(Encode_salinity(s)), s, 0.11));

    // saturates rather than wrapping
    assert(Encode_salinity(1000.0f) == 255);
}

void check_depth()
{
    assert(Encode_depth(-1.0f) == 0);
    assert(Encode_depth(10000.0f) == 8100);

    // each of the four resolution bands round trips within its own resolution
    assert(close(Decode_depth(Encode_depth(0.0f)), 0.0, 0.05));
    assert(close(Decode_depth(Encode_depth(50.0f)), 50.0, 0.05));
    assert(close(Decode_depth(Encode_depth(99.0f)), 99.0, 0.05));
    assert(close(Decode_depth(Encode_depth(150.0f)), 150.0, 0.1));
    assert(close(Decode_depth(Encode_depth(500.0f)), 500.0, 0.25));
    assert(close(Decode_depth(Encode_depth(3000.0f)), 3000.0, 0.5));

    // decode masks to 13 bits, so anything above saturates at 6000
    assert(close(Decode_depth(8100), 6000.0, 1e-3));
}

void check_temperature()
{
    // clamps at both ends of the -4 to +36 C range
    assert(Encode_temperature(-100.0f) == 0);
    assert(Encode_temperature(1000.0f) == 255);

    for (float t : {-4.0f, 0.0f, 10.0f, 20.0f, 35.0f})
        assert(close(Decode_temperature(Encode_temperature(t)), t, 0.09));
}

void check_sound_speed()
{
    for (float c : {1425.0f, 1450.0f, 1500.0f, 1550.0f})
        assert(close(Decode_sound_speed(Encode_sound_speed(c)), c, 0.5));

    assert(Encode_sound_speed(1425.0f) == 0);
}

void check_hires_altitude()
{
    assert(Encode_hires_altitude(-1.0f) == 0);
    assert(Encode_hires_altitude(1000.0f) == 65535U); // saturates past ~655 m

    for (float a : {0.0f, 1.5f, 100.0f, 655.0f})
        assert(close(Decode_hires_altitude(Encode_hires_altitude(a)), a, 0.01));
}

void check_gfi_pitch_oil()
{
    float gfi = 0, pitch = 0, oil = 0;

    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(50.0f, 30.0f, 75.0f), &gfi, &pitch, &oil);
    assert(close(gfi, 50.0, 3.5));
    assert(close(pitch, 30.0, 3.0));
    assert(close(oil, 75.0, 3.5));

    // negative pitch must survive the packed sign
    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(0.0f, -45.0f, 0.0f), &gfi, &pitch, &oil);
    assert(close(pitch, -45.0, 3.0));
    assert(close(gfi, 0.0, 3.5));
    assert(close(oil, 0.0, 3.5));

    // gfi and oil clamp to their documented 0-100 range
    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(1000.0f, 0.0f, 1000.0f), &gfi, &pitch, &oil);
    assert(close(gfi, 100.0, 3.5));
    assert(close(oil, 100.0, 3.5));

    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(-100.0f, 0.0f, -100.0f), &gfi, &pitch, &oil);
    assert(close(gfi, 0.0, 3.5));
    assert(close(oil, 0.0, 3.5));

    // pitch is good up to just inside the clamp endpoints
    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(0.0f, 89.0f, 0.0f), &gfi, &pitch, &oil);
    assert(close(pitch, 89.0, 3.0));
    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(0.0f, -89.0f, 0.0f), &gfi, &pitch, &oil);
    assert(close(pitch, -89.0, 3.0));

    // Pinning a known defect in this legacy wire format rather than a desired
    // result: the pitch field holds 6 signed bits (-32..31), but the clamp
    // endpoints scale to +/-32, so both +90 and -90 encode to 0x8000 and come
    // back as -91.4. Fixing it would change the CCL wire format.
    assert(Encode_gfi_pitch_oil(0.0f, 90.0f, 0.0f) == Encode_gfi_pitch_oil(0.0f, -90.0f, 0.0f));
    Decode_gfi_pitch_oil(Encode_gfi_pitch_oil(0.0f, 90.0f, 0.0f), &gfi, &pitch, &oil);
    assert(close(pitch, -91.4286, 1e-3));
}

void check_time_date()
{
    // 2021-03-04 05:06:08 UTC; seconds have 4-second resolution and the year
    // is not encoded at all
    struct tm when = {};
    when.tm_year = 2021 - 1900;
    when.tm_mon = 2;
    when.tm_mday = 4;
    when.tm_hour = 5;
    when.tm_min = 6;
    when.tm_sec = 8;
    long secs = static_cast<long>(timegm(&when));

    short mon = 0, day = 0, hour = 0, min = 0, sec = 0;
    Decode_time_date(Encode_time_date(secs), &mon, &day, &hour, &min, &sec);

    assert(mon == 3);
    assert(day == 4);
    assert(hour == 5);
    assert(min == 6);
    assert(sec == 8);

    // the epoch itself, to exercise the all-zero-ish path
    Decode_time_date(Encode_time_date(0), &mon, &day, &hour, &min, &sec);
    assert(mon == 1);
    assert(day == 1);
    assert(hour == 0);
    assert(min == 0);
    assert(sec == 0);
}

void check_watts()
{
    assert(Encode_watts(0.0f, 0.0f) == 0);
    assert(Encode_watts(-10.0f, 10.0f) == 0);      // negative clamps to 0
    assert(Encode_watts(1000.0f, 1000.0f) == 255); // saturates

    assert(close(Decode_watts(Encode_watts(10.0f, 10.0f)), 100.0, 4.0));
    assert(Decode_watts(255) == 1020.0f);
}

void check_speed()
{
    for (float rpm : {0.0f, 50.0f, 127.0f, -127.0f})
        assert(close(Decode_speed(SPEED_MODE_RPM, Encode_speed(SPEED_MODE_RPM, rpm / 20.0f)), rpm,
                     20.0));

    for (float msec : {0.0f, 1.0f, 2.5f, 4.2f, -2.0f})
        assert(
            close(Decode_speed(SPEED_MODE_MSEC, Encode_speed(SPEED_MODE_MSEC, msec)), msec, 0.04));

    // both modes clamp to the signed byte range
    assert(Encode_speed(SPEED_MODE_RPM, 1000.0f) == 127);
    assert(Encode_speed(SPEED_MODE_RPM, -1000.0f) == -127);
    assert(Encode_speed(SPEED_MODE_MSEC, 1000.0f) == 127);
    assert(Encode_speed(SPEED_MODE_MSEC, -1000.0f) == -127);

    // knots is accepted on encode but explicitly unsupported on decode
    assert(Decode_speed(SPEED_MODE_KNOTS, 42) == 0);
}

void check_ranger_bcd()
{
    assert(close(DecodeRangerBCD(0x00), 0.0, 1e-9));
    assert(close(DecodeRangerBCD(0x42), 42.0, 1e-9));
    assert(close(DecodeRangerBCD(0x99), 99.0, 1e-9));

    assert(close(DecodeRangerBCD2(0x12, 0x34), 1234.0, 1e-9));
    assert(close(DecodeRangerBCD2(0x00, 0x07), 7.0, 1e-9));

    // 41 deg 23.4567 min north
    assert(close(DecodeRangerLL(0x04, 0x1e, 0x23, 0x45, 0x67), 41.0 + 23.4567 / 60.0, 1e-9));
    // the 0x0c/0x0d sign nibbles mark a negative value
    assert(close(DecodeRangerLL(0x04, 0x1c, 0x23, 0x45, 0x67), -(41.0 + 23.4567 / 60.0), 1e-9));
    assert(close(DecodeRangerLL(0x04, 0x1d, 0x23, 0x45, 0x67), -(41.0 + 23.4567 / 60.0), 1e-9));
    // three-digit degrees
    assert(close(DecodeRangerLL(0x12, 0x3e, 0x00, 0x00, 0x00), 123.0, 1e-9));
}
} // namespace

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    check_latlon();
    check_heading();
    check_est_velocity();
    check_salinity();
    check_depth();
    check_temperature();
    check_sound_speed();
    check_hires_altitude();
    check_gfi_pitch_oil();
    check_time_date();
    check_watts();
    check_speed();
    check_ranger_bcd();

    std::cout << "all tests passed" << std::endl;
    return 0;
}
