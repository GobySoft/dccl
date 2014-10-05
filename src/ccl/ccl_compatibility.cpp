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

struct CodecLoader
{
    CodecLoader()
        {
            using namespace dccl;
            using namespace dccl::legacyccl;
            FieldCodecManager::add<IdentifierCodec>("dccl.ccl.id");

            FieldCodecManager::add<LatLonCompressedCodec>("_ccl_latloncompressed");
            FieldCodecManager::add<FixAgeCodec>("_ccl_fix_age");
            FieldCodecManager::add<TimeDateCodec>("_ccl_time_date");
            FieldCodecManager::add<HeadingCodec>("_ccl_heading");
            FieldCodecManager::add<DepthCodec>("_ccl_depth");
            FieldCodecManager::add<VelocityCodec>("_ccl_velocity");
            FieldCodecManager::add<WattsCodec>("_ccl_watts");
            FieldCodecManager::add<GFIPitchOilCodec>("_ccl_gfi_pitch_oil");
            FieldCodecManager::add<SpeedCodec>("_ccl_speed");
            FieldCodecManager::add<HiResAltitudeCodec>("_ccl_hires_altitude");
            FieldCodecManager::add<TemperatureCodec>("_ccl_temperature");
            FieldCodecManager::add<SalinityCodec>("_ccl_salinity");
            FieldCodecManager::add<SoundSpeedCodec>("_ccl_sound_speed");
        }
    ~CodecLoader()
        {
            using namespace dccl;
            using namespace dccl::legacyccl;
            FieldCodecManager::remove<IdentifierCodec>("dccl.ccl.id");

            FieldCodecManager::remove<LatLonCompressedCodec>("_ccl_latloncompressed");
            FieldCodecManager::remove<FixAgeCodec>("_ccl_fix_age");
            FieldCodecManager::remove<TimeDateCodec>("_ccl_time_date");
            FieldCodecManager::remove<HeadingCodec>("_ccl_heading");
            FieldCodecManager::remove<DepthCodec>("_ccl_depth");
            FieldCodecManager::remove<VelocityCodec>("_ccl_velocity");
            FieldCodecManager::remove<WattsCodec>("_ccl_watts");
            FieldCodecManager::remove<GFIPitchOilCodec>("_ccl_gfi_pitch_oil");
            FieldCodecManager::remove<SpeedCodec>("_ccl_speed");
            FieldCodecManager::remove<HiResAltitudeCodec>("_ccl_hires_altitude");
            FieldCodecManager::remove<TemperatureCodec>("_ccl_temperature");
            FieldCodecManager::remove<SalinityCodec>("_ccl_salinity");
            FieldCodecManager::remove<SoundSpeedCodec>("_ccl_sound_speed");
        }
};
    
static CodecLoader loader;

extern "C"
{
    void dccl3_load(dccl::Codec* dccl)
    {        
        dccl->load<dccl::legacyccl::protobuf::CCLMDATEmpty>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATRedirect>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATBathy>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATCTD>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATState>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATCommand>();
        dccl->load<dccl::legacyccl::protobuf::CCLMDATError>();
    }
}

//
// LatLonCompressedCodec
//

dccl::Bitset dccl::legacyccl::LatLonCompressedCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::legacyccl::LatLonCompressedCodec::encode(const double& wire_value)
{
    LONG_AND_COMP encoded;
    encoded.as_long = 0;
    encoded.as_compressed = Encode_latlon(wire_value);
    return dccl::Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

double dccl::legacyccl::LatLonCompressedCodec::decode(Bitset* bits)
{    
    LONG_AND_COMP decoded;
    decoded.as_long = static_cast<long>(bits->to_ulong());
    return Decode_latlon(decoded.as_compressed);
}

unsigned dccl::legacyccl::LatLonCompressedCodec::size()
{
    return LATLON_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}

//
// TimeDateCodec
//

dccl::Bitset dccl::legacyccl::TimeDateCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::legacyccl::TimeDateCodec::encode(const dccl::uint64& wire_value)
{
    TIME_DATE_LONG encoded;
    encoded.as_long = 0;
    encoded.as_time_date = Encode_time_date(wire_value / MICROSECONDS_IN_SECOND);
    return dccl::Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

dccl::uint64 dccl::legacyccl::TimeDateCodec::decode(Bitset* bits)
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

dccl::uint64 dccl::legacyccl::TimeDateCodec::to_uint64_time(const boost::posix_time::ptime& time_date)
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


unsigned dccl::legacyccl::TimeDateCodec::size()
{
    return TIME_DATE_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}


//
// HeadingCodec
//
dccl::Bitset dccl::legacyccl::HeadingCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_heading(wire_value)); } 

float dccl::legacyccl::HeadingCodec::decode(Bitset* bits)
{ return Decode_heading(bits->to_ulong()); }


//
// DepthCodec
//
dccl::Bitset dccl::legacyccl::DepthCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_depth(wire_value)); } 

float dccl::legacyccl::DepthCodec::decode(Bitset* bits)
{ return Decode_depth(bits->to_ulong()); }

//
// VelocityCodec
//
dccl::Bitset dccl::legacyccl::VelocityCodec::encode(const float& wire_value)
{
    return dccl::Bitset(size(), Encode_est_velocity(wire_value));
} 

float dccl::legacyccl::VelocityCodec::decode(Bitset* bits)
{ return Decode_est_velocity(bits->to_ulong()); }


//
// SpeedCodec
//
dccl::Bitset dccl::legacyccl::SpeedCodec::encode(const float& wire_value)
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

float dccl::legacyccl::SpeedCodec::decode(Bitset* bits)
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
// WattsCodec
//
dccl::Bitset dccl::legacyccl::WattsCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_watts(wire_value, 1)); } 

float dccl::legacyccl::WattsCodec::decode(Bitset* bits)
{ return Decode_watts(bits->to_ulong()); }

//
// GFIPitchOilCodec
//
dccl::Bitset dccl::legacyccl::GFIPitchOilCodec::encode(const protobuf::CCLMDATState::GFIPitchOil& wire_value)
{
    return dccl::Bitset(size(), Encode_gfi_pitch_oil(wire_value.gfi(), wire_value.pitch(), wire_value.oil()));
}

dccl::legacyccl::protobuf::CCLMDATState::GFIPitchOil dccl::legacyccl::GFIPitchOilCodec::decode(Bitset* bits)
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
// HiResAltitudeCodec
//
dccl::Bitset dccl::legacyccl::HiResAltitudeCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_hires_altitude(wire_value)); } 

float dccl::legacyccl::HiResAltitudeCodec::decode(Bitset* bits)
{ return Decode_hires_altitude(bits->to_ulong()); }

//
// SalinityCodec
//
dccl::Bitset dccl::legacyccl::SalinityCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_salinity(wire_value)); } 

float dccl::legacyccl::SalinityCodec::decode(Bitset* bits)
{ return Decode_salinity(bits->to_ulong()); }

//
// TemperatureCodec
//
dccl::Bitset dccl::legacyccl::TemperatureCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_temperature(wire_value)); } 

float dccl::legacyccl::TemperatureCodec::decode(Bitset* bits)
{ return Decode_temperature(bits->to_ulong()); }

//
// SoundSpeedCodec
//
dccl::Bitset dccl::legacyccl::SoundSpeedCodec::encode(const float& wire_value)
{ return dccl::Bitset(size(), Encode_sound_speed(wire_value)); } 

float dccl::legacyccl::SoundSpeedCodec::decode(Bitset* bits)
{ return Decode_sound_speed(bits->to_ulong()); }
