// Copyright 2012-2017:
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
/* Latitude/Longitude 
 * Each are encoded into 3 bytes, giving a resolution of
 * about 2 meters. The compressed form is stored in a LATLON_COMPRESSED struct, 
 * which is 3 bytes. 
*/
typedef struct //NOLINT
{
    char compressed[3];
} LATLON_COMPRESSED;

typedef union { //NOLINT
    long as_long;
    LATLON_COMPRESSED as_compressed;
}

LONG_AND_COMP;

typedef struct //NOLINT
{
    unsigned char compressed[3];
} TIME_DATE;

typedef union { //NOLINT
    long as_long;
    TIME_DATE as_time_date;
} TIME_DATE_LONG;

typedef enum //NOLINT
{
    SPEED_MODE_RPM = 0,
    SPEED_MODE_MSEC = 1,
    SPEED_MODE_KNOTS = 2
} SPEED_MODE;

typedef enum //NOLINT
{
    PS_DISABLED = 0,
    PS_SHIPS_POLE = 1,
    PS_GATEWAY_BUOY = 2,      // "automatically" set...
    PS_NAMED_TRANSPONDER = 3, // could be a drifter, or moored; label indicates who...
    PS_VEHICLE_POSITION = 4,
    PS_LAST = 5, // Pay no attention to the PS_INVALID behind the curtain...
    PS_INVALID =
        0x080 // Or'd in to indicate that the systems position is (temporarily) invalid (ship turning, etc.)
} POSITION_SEND;

LATLON_COMPRESSED Encode_latlon(double latlon_in);
double Decode_latlon(LATLON_COMPRESSED latlon_in);
unsigned char Encode_heading(float heading);
double Decode_heading(unsigned char heading);
char Encode_est_velocity(float est_velocity);
float Decode_est_velocity(char est_velocity);
unsigned char Encode_salinity(float salinity);
float Decode_salinity(unsigned char sal);
unsigned short Encode_depth(float depth);
float Decode_depth(unsigned short depth);
unsigned char Encode_temperature(float temperature);
float Decode_temperature(unsigned char temperature);
unsigned char Encode_sound_speed(float sound_speed);
float Decode_sound_speed(unsigned char sound_speed);
unsigned short Encode_hires_altitude(float alt);
float Decode_hires_altitude(unsigned short alt);
unsigned short Encode_gfi_pitch_oil(float gfi, float pitch, float oil);
void Decode_gfi_pitch_oil(unsigned short gfi_pitch_oil, float* gfi, float* pitch, float* oil);
TIME_DATE Encode_time_date(long secs_since_1970);
long Decode_time_date(TIME_DATE input, short* mon, short* day, short* hour, short* min, short* sec);
unsigned char Encode_watts(float volts, float amps);
float Decode_watts(unsigned char watts_encoded);
char Encode_speed(SPEED_MODE mode, float speed);
float Decode_speed(SPEED_MODE mode, char speed);

double DecodeRangerLL(unsigned char c1, unsigned char c2, unsigned char c3, unsigned char c4,
                      unsigned char c5);
double DecodeRangerBCD2(unsigned char c1, unsigned char c2);
double DecodeRangerBCD(unsigned char c1);
