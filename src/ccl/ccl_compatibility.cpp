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


#include <ctime>

#include <boost/date_time.hpp>

#include "ccl_compatibility.h"
#include "WhoiUtil.h"
#include "dccl/codec.h"

// shared library load


extern "C"
{
    void dccl3_load(dccl::Codec* dccl)
    {
        using namespace dccl;
        
        FieldCodecManager::add<LegacyCCLLatLonCompressedCodec>("_ccl_latloncompressed");
        FieldCodecManager::add<LegacyCCLFixAgeCodec>("_ccl_fix_age");
        FieldCodecManager::add<LegacyCCLTimeDateCodec>("_ccl_time_date");
        FieldCodecManager::add<LegacyCCLHeadingCodec>("_ccl_heading");
        FieldCodecManager::add<LegacyCCLDepthCodec>("_ccl_depth");
        FieldCodecManager::add<LegacyCCLVelocityCodec>("_ccl_velocity");
        FieldCodecManager::add<LegacyCCLWattsCodec>("_ccl_watts");
        FieldCodecManager::add<LegacyCCLGFIPitchOilCodec>("_ccl_gfi_pitch_oil");
        FieldCodecManager::add<LegacyCCLSpeedCodec>("_ccl_speed");
        FieldCodecManager::add<LegacyCCLHiResAltitudeCodec>("_ccl_hires_altitude");
        FieldCodecManager::add<LegacyCCLTemperatureCodec>("_ccl_temperature");
        FieldCodecManager::add<LegacyCCLSalinityCodec>("_ccl_salinity");
        FieldCodecManager::add<LegacyCCLSoundSpeedCodec>("_ccl_sound_speed");
        
        dccl->load<dccl::protobuf::CCLMDATEmpty>();
        dccl->load<dccl::protobuf::CCLMDATRedirect>();
        dccl->load<dccl::protobuf::CCLMDATBathy>();
        dccl->load<dccl::protobuf::CCLMDATCTD>();
        dccl->load<dccl::protobuf::CCLMDATState>();
        dccl->load<dccl::protobuf::CCLMDATCommand>();
        dccl->load<dccl::protobuf::CCLMDATError>();
    }
}

//
// LegacyCCLLatLonCompressedCodec
//

dccl::Bitset dccl::LegacyCCLLatLonCompressedCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::LegacyCCLLatLonCompressedCodec::encode(const double& wire_value)
{
    LONG_AND_COMP encoded;
    encoded.as_long = 0;
    encoded.as_compressed = Encode_latlon(wire_value);
    return dccl::Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

double dccl::LegacyCCLLatLonCompressedCodec::decode(Bitset* bits)
{    
    LONG_AND_COMP decoded;
    decoded.as_long = static_cast<long>(bits->to_ulong());
    return Decode_latlon(decoded.as_compressed);
}

unsigned dccl::LegacyCCLLatLonCompressedCodec::size()
{
    return LATLON_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}

//
// LegacyCCLTimeDateCodec
//

dccl::Bitset dccl::LegacyCCLTimeDateCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::LegacyCCLTimeDateCodec::encode(const dccl::uint64& wire_value)
{
    TIME_DATE_LONG encoded;
    encoded.as_long = 0;
    encoded.as_time_date = Encode_time_date(wire_value / MICROSECONDS_IN_SECOND);
    return dccl::Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

dccl::uint64 dccl::LegacyCCLTimeDateCodec::decode(Bitset* bits)
{
    TIME_DATE_LONG decoded;
    decoded.as_long = bits->to_ulong();
    short mon, day, hour, min, sec;
    Decode_time_date(decoded.as_time_date,
                     &mon, &day, &hour, &min, &sec);

    // \todo chrismurf FIX ME! with timegm
    // assume current year
    int year = boost::gregorian::day_clock::universal_day().year();

    boost::posix_time::ptime time_date(
        boost::gregorian::date(year, mon, day), 
        boost::posix_time::time_duration(hour,min,sec));

    return to_uint64_time(time_date);
}

dccl::uint64 dccl::LegacyCCLTimeDateCodec::to_uint64_time(const boost::posix_time::ptime& time_date)
{
            
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    
    if (time_date == not_a_date_time)
        return std::numeric_limits<uint64>::max();
    else
    {
        const int MICROSEC_IN_SEC = 1000000;

        date_duration date_diff = time_date.date() - date(1970,1,1);
        time_duration time_diff = time_date.time_of_day();
        
        return
            static_cast<uint64>(date_diff.days())*24*3600*MICROSEC_IN_SEC + 
            static_cast<uint64>(time_diff.total_seconds())*MICROSEC_IN_SEC +
            static_cast<uint64>(time_diff.fractional_seconds()) /
            (time_duration::ticks_per_second() / MICROSEC_IN_SEC);        
    }    
}


unsigned dccl::LegacyCCLTimeDateCodec::size()
{
    return TIME_DATE_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}


//
// LegacyCCLHeadingCodec
//
dccl::Bitset dccl::LegacyCCLHeadingCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_heading(wire_value)); } 

float dccl::LegacyCCLHeadingCodec::decode(Bitset* bits)
{ return Decode_heading(bits->to_ulong()); }


//
// LegacyCCLDepthCodec
//
dccl::Bitset dccl::LegacyCCLDepthCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_depth(wire_value)); } 

float dccl::LegacyCCLDepthCodec::decode(Bitset* bits)
{ return Decode_depth(bits->to_ulong()); }

//
// LegacyCCLVelocityCodec
//
dccl::Bitset dccl::LegacyCCLVelocityCodec::encode(const float& wire_value)
{
    return dccl::Bitset(size(), Encode_est_velocity(wire_value));
} 

float dccl::LegacyCCLVelocityCodec::decode(Bitset* bits)
{ return Decode_est_velocity(bits->to_ulong()); }


//
// LegacyCCLSpeedCodec
//
dccl::Bitset dccl::LegacyCCLSpeedCodec::encode(const float& wire_value)
{
    const google::protobuf::Message* root = FieldCodecBase::root_message();
    const google::protobuf::FieldDescriptor* thrust_mode_field_desc =
        root->GetDescriptor()->FindFieldByNumber(
            FieldCodecBase::dccl_field_options().GetExtension(ccl).thrust_mode_tag());

    switch(root->GetReflection()->GetEnum(*root, thrust_mode_field_desc)->number())
    {
        default:
        case protobuf::CCLMDATRedirect::RPM:
            return dccl::Bitset(size(), Encode_speed(SPEED_MODE_RPM, wire_value));
            
        case protobuf::CCLMDATRedirect::METERS_PER_SECOND:
            return dccl::Bitset(size(), Encode_speed(SPEED_MODE_MSEC, wire_value));
    }
} 

float dccl::LegacyCCLSpeedCodec::decode(Bitset* bits)
{
    const google::protobuf::Message* root = FieldCodecBase::root_message();
    const google::protobuf::FieldDescriptor* thrust_mode_field_desc =
        root->GetDescriptor()->FindFieldByNumber(
            FieldCodecBase::dccl_field_options().GetExtension(ccl).thrust_mode_tag());

    switch(root->GetReflection()->GetEnum(*root, thrust_mode_field_desc)->number())
    {
        default:
        case protobuf::CCLMDATRedirect::RPM:
            return Decode_speed(SPEED_MODE_RPM, bits->to_ulong());
            
        case protobuf::CCLMDATRedirect::METERS_PER_SECOND:
            return Decode_speed(SPEED_MODE_MSEC, bits->to_ulong());
    }
}



//
// LegacyCCLWattsCodec
//
dccl::Bitset dccl::LegacyCCLWattsCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_watts(wire_value, 1)); } 

float dccl::LegacyCCLWattsCodec::decode(Bitset* bits)
{ return Decode_watts(bits->to_ulong()); }

//
// LegacyCCLGFIPitchOilCodec
//
dccl::Bitset dccl::LegacyCCLGFIPitchOilCodec::encode(const protobuf::CCLMDATState::GFIPitchOil& wire_value)
{
    return dccl::Bitset(size(), Encode_gfi_pitch_oil(wire_value.gfi(), wire_value.pitch(), wire_value.oil()));
}

dccl::protobuf::CCLMDATState::GFIPitchOil dccl::LegacyCCLGFIPitchOilCodec::decode(Bitset* bits)
{
    float gfi, pitch, oil;
    Decode_gfi_pitch_oil(bits->to_ulong(), &gfi, &pitch, &oil);
    protobuf::CCLMDATState::GFIPitchOil decoded;
    decoded.set_gfi(gfi);
    decoded.set_pitch(pitch);
    decoded.set_oil(oil);
    return decoded;
}

//
// LegacyCCLHiResAltitudeCodec
//
dccl::Bitset dccl::LegacyCCLHiResAltitudeCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_hires_altitude(wire_value)); } 

float dccl::LegacyCCLHiResAltitudeCodec::decode(Bitset* bits)
{ return Decode_hires_altitude(bits->to_ulong()); }

//
// LegacyCCLSalinityCodec
//
dccl::Bitset dccl::LegacyCCLSalinityCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_salinity(wire_value)); } 

float dccl::LegacyCCLSalinityCodec::decode(Bitset* bits)
{ return Decode_salinity(bits->to_ulong()); }

//
// LegacyCCLTemperatureCodec
//
dccl::Bitset dccl::LegacyCCLTemperatureCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_temperature(wire_value)); } 

float dccl::LegacyCCLTemperatureCodec::decode(Bitset* bits)
{ return Decode_temperature(bits->to_ulong()); }

//
// LegacyCCLSoundSpeedCodec
//
dccl::Bitset dccl::LegacyCCLSoundSpeedCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_sound_speed(wire_value)); } 

float dccl::LegacyCCLSoundSpeedCodec::decode(Bitset* bits)
{ return Decode_sound_speed(bits->to_ulong()); }
