// Copyright 2011-2023:
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
// tests usage of Legacy CCL

#include "../../ccl/ccl_compatibility.h"
#include "../../codec.h"
#include "../../field_codec.h"
#include "test.pb.h"

using namespace dccl::test;

using dccl::operator<<;

bool double_cmp(double a, double b, int precision)
{
    int a_whole = a;
    int b_whole = b;

    int a_part = (a - a_whole) * pow(10.0, precision);
    int b_part = (b - b_whole) * pow(10.0, precision);

    return (a_whole == b_whole) && (a_part == b_part);
}

template <typename N> void check_normal_dccl(dccl::Codec& codec)
{
    codec.load<N>();
    codec.info<N>(&dccl::dlog);
    N normal_msg, normal_msg_out;
    normal_msg.set_a(123);
    normal_msg.set_b(321);

    std::string encoded;
    codec.encode(&encoded, normal_msg);
    std::cout << dccl::hex_encode(encoded) << std::endl;
    assert(dccl::hex_encode(encoded).substr(0, 2) == "20");
    codec.decode(encoded, &normal_msg_out);

    assert(normal_msg.SerializeAsString() == normal_msg_out.SerializeAsString());
}

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec("dccl.ccl.id", DCCL_CCL_COMPAT_NAME);

    check_normal_dccl<NormalDCCL1Byte>(codec);
    check_normal_dccl<NormalDCCL2Byte>(codec);

    codec.info<dccl::legacyccl::protobuf::CCLMDATState>(&dccl::dlog);

    dccl::legacyccl::protobuf::CCLMDATState state_in, state_out;
    std::string test_state_encoded =
        "0e86fa11ad20c9011b4432bf47d10000002401042f0e7d87fa111620c95a200a";
    codec.decode(dccl::hex_decode(test_state_encoded), &state_out);
    state_in.set_latitude(25.282416667);
    state_in.set_longitude(-77.164266667);
    state_in.set_fix_age(4);

    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm input_tm = *std::gmtime(&now_time_t); // get current year
    input_tm.tm_mon = 2;                          // zero indexed March
    input_tm.tm_mday = 4;
    input_tm.tm_hour = 17;
    input_tm.tm_min = 1;
    input_tm.tm_sec = 44;
    input_tm.tm_isdst = -1;
    std::time_t time_date = timegm(&input_tm);

    state_in.set_time_date(dccl::legacyccl::TimeDateCodec::to_uint64_time(time_date));
    state_in.set_heading(270);
    state_in.set_depth(2323);
    state_in.set_mission_mode(dccl::legacyccl::protobuf::CCLMDATState::NORMAL);
    state_in.set_faults(dccl::hex_decode("00000024"));
    state_in.set_faults_2(dccl::hex_decode("01"));
    state_in.set_mission_leg(4);
    state_in.set_est_velocity(1.88);
    state_in.set_objective_index(dccl::hex_decode("0E"));
    state_in.set_watts(500);
    state_in.set_lat_goal(25.282440815262891);
    state_in.set_lon_goal(-77.167505880296929);
    state_in.set_battery_percent(90);
    state_in.mutable_gfi_pitch_oil()->set_gfi(0);
    state_in.mutable_gfi_pitch_oil()->set_pitch(6);
    state_in.mutable_gfi_pitch_oil()->set_oil(55);

    assert(double_cmp(state_in.latitude(), state_out.latitude(), 4));
    assert(double_cmp(state_in.longitude(), state_out.longitude(), 4));
    assert(state_in.fix_age() == state_out.fix_age());
    assert(state_in.time_date() == state_out.time_date());
    assert(dccl::round(state_in.heading(), 0) == dccl::round(state_out.heading(), 0));
    assert(double_cmp(state_in.depth(), state_out.depth(), 1));
    assert(state_in.mission_mode() == state_out.mission_mode());

    // test the dynamically generated message
    std::shared_ptr<google::protobuf::Message> state_in_2 =
        dccl::DynamicProtobufManager::new_protobuf_message(
            dccl::legacyccl::protobuf::CCLMDATState::descriptor());
    state_in_2->CopyFrom(state_in);

    std::string state_encoded;
    codec.encode(&state_encoded, *state_in_2);

    dccl::legacyccl::protobuf::CCLMDATState state_out_2;
    codec.decode(state_encoded, &state_out_2);

    std::cout << "in:" << state_in << std::endl;
    std::cout << test_state_encoded << std::endl;
    std::cout << dccl::hex_encode(state_encoded) << std::endl;
    std::cout << std::setprecision(16) << state_out.lon_goal() << std::endl;
    std::cout << "out:" << state_out << std::endl;
    std::cout << "out2: " << state_out_2 << std::endl;

    assert(state_out.SerializeAsString() == state_out_2.SerializeAsString());
    assert(test_state_encoded == dccl::hex_encode(state_encoded));

    std::cout << dccl::hex_encode(state_out.faults()) << std::endl;
    std::cout << dccl::hex_encode(state_out.faults_2()) << std::endl;

    codec.info<dccl::legacyccl::protobuf::CCLMDATRedirect>(&dccl::dlog);

    dccl::legacyccl::protobuf::CCLMDATRedirect redirect_in, redirect_out;
    std::string test_redirect_encoded =
        "07522cf9113d20c99964003d6464003d640be60014142035f911ef21c9000000";
    codec.decode(dccl::hex_decode(test_redirect_encoded), &redirect_out);
    redirect_in.set_message_number(82);
    redirect_in.set_latitude(25.274995002149939);
    redirect_in.set_longitude(-77.166669030984522);
    redirect_in.set_transit_vertical_mode(dccl::legacyccl::protobuf::CCLMDATRedirect::ALTITUDE);
    redirect_in.set_transit_thrust_mode(
        dccl::legacyccl::protobuf::CCLMDATRedirect::METERS_PER_SECOND);
    redirect_in.set_survey_vertical_mode(dccl::legacyccl::protobuf::CCLMDATRedirect::ALTITUDE);
    redirect_in.set_survey_thrust_mode(
        dccl::legacyccl::protobuf::CCLMDATRedirect::METERS_PER_SECOND);

    redirect_in.set_depth_goal_transit(10.0);
    redirect_in.set_speed_transit(2.0333333);
    redirect_in.set_device_cmd_transit(100);

    redirect_in.set_depth_goal_survey(10.0);
    redirect_in.set_speed_survey(2.0333333);
    redirect_in.set_device_cmd_survey(100);

    redirect_in.set_num_rows(11);
    redirect_in.set_row_length(230);
    redirect_in.set_spacing_0(20);
    redirect_in.set_spacing_1(20);
    redirect_in.set_heading(45.176472);

    redirect_in.set_lat_start(25.275183333);
    redirect_in.set_lon_start(-77.15735);

    redirect_in.set_spare(std::string(3, '\0'));

    std::string redirect_encoded;
    codec.encode(&redirect_encoded, redirect_in);

    dccl::legacyccl::protobuf::CCLMDATRedirect redirect_out_2;
    codec.decode(redirect_encoded, &redirect_out_2);

    std::cout << "in:" << redirect_in << std::endl;
    std::cout << test_redirect_encoded << std::endl;
    std::cout << dccl::hex_encode(redirect_encoded) << std::endl;
    std::cout << "out:" << redirect_out << std::endl;
    std::cout << "out2: " << redirect_out_2 << std::endl;

    assert(redirect_out.SerializeAsString() == redirect_out_2.SerializeAsString());
    assert(test_redirect_encoded == dccl::hex_encode(redirect_encoded));

    codec.info<dccl::legacyccl::protobuf::CCLMDATEmpty>(&dccl::dlog);
    codec.info<dccl::legacyccl::protobuf::CCLMDATBathy>(&dccl::dlog);
    codec.info<dccl::legacyccl::protobuf::CCLMDATCTD>(&dccl::dlog);
    codec.info<dccl::legacyccl::protobuf::CCLMDATError>(&dccl::dlog);
    codec.info<dccl::legacyccl::protobuf::CCLMDATCommand>(&dccl::dlog);

    std::cout << "all tests passed" << std::endl;
}
