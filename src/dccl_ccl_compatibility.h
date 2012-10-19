// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#ifndef DCCLCCLCOMPATIBILITY20120426H
#define DCCLCCLCOMPATIBILITY20120426H

#include "dccl_field_codec_default.h"
#include "goby/acomms/acomms_constants.h"
#include "dccl/protobuf/ccl.pb.h"
#include "dccl/protobuf/ccl_extensions.pb.h"

extern "C"
{
    void goby_dccl_load(goby::acomms::DCCLCodec* dccl);
}



namespace dccl
{
    class LegacyCCLIdentifierCodec : public goby::acomms::DCCLDefaultIdentifierCodec
    {
      private:
        goby::acomms::Bitset encode()
        { return encode(0); }
            
        goby::acomms::Bitset encode(const goby::uint32& wire_value)
        {
            if((wire_value & 0xFFFF0000) == CCL_DCCL_ID_PREFIX)
            {
                // CCL message
                return goby::acomms::Bitset(goby::acomms::BITS_IN_BYTE, wire_value & 0x0000FFFF);
            }
            else
            {
                // DCCL message
                return goby::acomms::DCCLDefaultIdentifierCodec::encode(wire_value).prepend(
                    goby::acomms::Bitset(goby::acomms::BITS_IN_BYTE, goby::acomms::DCCL_CCL_HEADER));
            }
                
        }
                
        goby::uint32 decode(goby::acomms::Bitset* bits)
        {
            unsigned ccl_id = bits->to_ulong();
                
            if(ccl_id == goby::acomms::DCCL_CCL_HEADER)
            {
                // DCCL message
                bits->get_more_bits(goby::acomms::DCCLDefaultIdentifierCodec::min_size());
                (*bits) >>= goby::acomms::BITS_IN_BYTE;
                return goby::acomms::DCCLDefaultIdentifierCodec::decode(bits);
            }
            else
            {
                // CCL message
                return CCL_DCCL_ID_PREFIX + ccl_id;
            }
        }
            
        unsigned size()
        { return size(0); }
            
        unsigned size(const goby::uint32& field_value)
        {
            if((field_value & 0xFFFF0000) == CCL_DCCL_ID_PREFIX)
            {
                // CCL message
                return goby::acomms::BITS_IN_BYTE;
            }
            else
            {
                return goby::acomms::BITS_IN_BYTE +
                    goby::acomms::DCCLDefaultIdentifierCodec::size(field_value);
            }
        }
            
        unsigned max_size()
        { return goby::acomms::BITS_IN_BYTE + goby::acomms::DCCLDefaultIdentifierCodec::max_size(); }

        unsigned min_size()
        { return goby::acomms::BITS_IN_BYTE; }

        // prefixes (goby.msg).dccl.id to indicate that this DCCL
        // message is an encoding of a legacy CCL message
        enum { CCL_DCCL_ID_PREFIX = 0x0CC10000 };
    };

    class LegacyCCLLatLonCompressedCodec : public goby::acomms::DCCLTypedFixedFieldCodec<double>
    {
      private:
        goby::acomms::Bitset encode();
        goby::acomms::Bitset encode(const double& wire_value);
        double decode(goby::acomms::Bitset* bits);
        unsigned size();
        enum { LATLON_COMPRESSED_BYTE_SIZE = 3 };            
    };

    class LegacyCCLFixAgeCodec : public goby::acomms::DCCLDefaultNumericFieldCodec<goby::uint32>
    {
      private:
        goby::acomms::Bitset encode()
        {
            return encode(max());
        }
            
        goby::acomms::Bitset encode(const goby::uint32& wire_value)
        {
            return DCCLDefaultNumericFieldCodec<goby::uint32>::encode(
                std::min<unsigned char>(max(), wire_value / SCALE_FACTOR));
        }
            
        goby::uint32 decode(goby::acomms::Bitset* bits)
        {
            return SCALE_FACTOR *
                DCCLDefaultNumericFieldCodec<goby::uint32>::decode(bits);
        }
                        
        double max() { return (1 << goby::acomms::BITS_IN_BYTE) - 1; }
        double min() { return 0; }
        void validate() { }
            
        enum { SCALE_FACTOR = 4 };
            
    };
        
            
    class LegacyCCLTimeDateCodec : public goby::acomms::DCCLTypedFixedFieldCodec<goby::uint64>
    {
      private:
        goby::acomms::Bitset encode();
        goby::acomms::Bitset encode(const goby::uint64& wire_value);
        goby::uint64 decode(goby::acomms::Bitset* bits);
        unsigned size();

        enum { MICROSECONDS_IN_SECOND = 1000000 };
        enum { TIME_DATE_COMPRESSED_BYTE_SIZE = 3 };
            
                
            
    };

    class LegacyCCLHeadingCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };


    class LegacyCCLHiResAltitudeCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size()
        {
            return HI_RES_ALTITUDE_COMPRESSED_BYTE_SIZE*goby::acomms::BITS_IN_BYTE;
        }
        enum { HI_RES_ALTITUDE_COMPRESSED_BYTE_SIZE = 2 };

    };

        
    class LegacyCCLDepthCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size()
        {
            return DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).bit_size();
        }
            
        void validate()
        {
            DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).has_bit_size(), "missing (goby.field).dccl.ccl.bit_size");
        }
            
    };

    class LegacyCCLVelocityCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };

    class LegacyCCLSpeedCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }

        void validate()
        {
            DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).has_thrust_mode_tag(), "missing (goby.field).dccl.ccl.thrust_mode_tag");
        }
    };


        
    class LegacyCCLWattsCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };

    class LegacyCCLGFIPitchOilCodec : public goby::acomms::DCCLTypedFixedFieldCodec<protobuf::CCLMDATState::GFIPitchOil>
    {
      private:
        goby::acomms::Bitset encode() { return encode(protobuf::CCLMDATState::GFIPitchOil()); }
        goby::acomms::Bitset encode(const protobuf::CCLMDATState::GFIPitchOil& wire_value);
        protobuf::CCLMDATState::GFIPitchOil decode(goby::acomms::Bitset* bits);
        unsigned size() { return GFI_PITCH_OIL_COMPRESSED_BYTE_SIZE*goby::acomms::BITS_IN_BYTE; }
        enum { GFI_PITCH_OIL_COMPRESSED_BYTE_SIZE =2};
                        
    };

    class LegacyCCLSalinityCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };

    class LegacyCCLTemperatureCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };

    class LegacyCCLSoundSpeedCodec : public goby::acomms::DCCLTypedFixedFieldCodec<float>
    {
      private:
        goby::acomms::Bitset encode() { return encode(0); }
        goby::acomms::Bitset encode(const float& wire_value);
        float decode(goby::acomms::Bitset* bits);
        unsigned size() { return goby::acomms::BITS_IN_BYTE; }
    };        
        
}

#endif
