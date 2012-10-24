// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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



// implements FieldCodecBase for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include <sys/time.h>

#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>

#include <google/protobuf/descriptor.h>

#include "dccl/protobuf/option_extensions.pb.h"

#include "field_codec_default_message.h"
#include "field_codec_fixed.h"
#include "field_codec.h"
#include "binary.h"

namespace dccl
{
    /// \brief Provides the default 1 byte or 2 byte DCCL ID codec
    class DefaultIdentifierCodec : public TypedFieldCodec<uint32>
    {
      protected:
        virtual Bitset encode();
        virtual Bitset encode(const uint32& wire_value);
        virtual uint32 decode(Bitset* bits);
        virtual unsigned size();
        virtual unsigned size(const uint32& wire_value);
        virtual unsigned max_size();
        virtual unsigned min_size();
        virtual void validate() { }

      private:
        unsigned this_size(const uint32& wire_value);
        // maximum id we can fit in short or long header (MSB reserved to indicate
        // short or long header)
        enum { ONE_BYTE_MAX_ID = (1 << 7) - 1,
               TWO_BYTE_MAX_ID = (1 << 15) - 1};
            
        enum { SHORT_FORM_ID_BYTES = 1,
               LONG_FORM_ID_BYTES = 2 };
    };

        

    /// \brief Provides a basic bounded arbitrary length numeric (double, float, uint32, uint64, int32, int64) encoder.
    ///
    /// Takes ceil(log2((max-min)*10^precision)+1) bits for required fields, ceil(log2((max-min)*10^precision)+2) for optional fields.
    template<typename WireType, typename FieldType = WireType>
        class DefaultNumericFieldCodec : public TypedFixedFieldCodec<WireType, FieldType>
    {
      protected:

      virtual double max()
      { return FieldCodecBase::dccl_field_options().max(); }

      virtual double min()
      { return FieldCodecBase::dccl_field_options().min(); }

      virtual double precision()
      { return FieldCodecBase::dccl_field_options().precision(); }
            
      virtual void validate()
      {
          FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_min(),
                                      "missing (dccl.field).min");
          FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_max(),
                                      "missing (dccl.field).max");


          // ensure given max and min fit within WireType ranges
          FieldCodecBase::require(min() >= boost::numeric::bounds<WireType>::lowest(),
                                      "(dccl.field).min must be >= minimum of this field type.");
          FieldCodecBase::require(max() <= boost::numeric::bounds<WireType>::highest(),
                                      "(dccl.field).max must be <= maximum of this field type.");
      }

      Bitset encode()
      {
          return Bitset(size());
      }
          
          
      virtual Bitset encode(const WireType& value)
      {
          WireType wire_value = value;
                
          if(wire_value < min() || wire_value > max())
              return Bitset(size());
              
              
//              dccl::dlog.is(common::logger::DEBUG2) && dccl::dlog << group(Codec::dlog_encode_group()) << "(DefaultNumericFieldCodec) Encoding using wire value (=field value) " << wire_value << std::endl;
              
          wire_value -= min();
          wire_value *= std::pow(10.0, precision());

          // "presence" value (0)
          if(!FieldCodecBase::this_field()->is_required())
              wire_value += 1;

          wire_value = dccl::unbiased_round(wire_value, 0);
          return Bitset(size(), boost::numeric_cast<unsigned long>(wire_value));
      }
          
      virtual WireType decode(Bitset* bits)
      {
          unsigned long t = bits->to_ulong();
              
          if(!FieldCodecBase::this_field()->is_required())
          {
              if(!t) throw(NullValueException());
              --t;
          }
              
          WireType return_value = dccl::unbiased_round(
              t / (std::pow(10.0, precision())) + min(), precision());
              
//              dccl::dlog.is(common::logger::DEBUG2) && dccl::dlog << group(Codec::dlog_decode_group()) << "(DefaultNumericFieldCodec) Decoding received wire value (=field value) " << return_value << std::endl;

          return return_value;
              
      }

      unsigned size()
      {
          // if not required field, leave one value for unspecified (always encoded as 0)
          const unsigned NULL_VALUE = FieldCodecBase::this_field()->is_required() ? 0 : 1;
              
          return dccl::ceil_log2((max()-min())*std::pow(10.0, precision())+1 + NULL_VALUE);
      }
            
    };

    /// \brief Provides a bool encoder. Uses 1 bit if field is `required`, 2 bits if `optional`
    ///
    /// [presence bit (0 bits if required, 1 bit if optional)][value (1 bit)]
    class DefaultBoolCodec : public TypedFixedFieldCodec<bool>
    {
      private:
        Bitset encode(const bool& wire_value);
        Bitset encode();
        bool decode(Bitset* bits);
        unsigned size();
        void validate();
    };
        
    /// \brief Provides an variable length ASCII string encoder. Can encode strings up to 255 bytes by using a length byte preceeding the string.
    ///
    /// [length of following string (1 byte)][string (0-255 bytes)]
    class DefaultStringCodec : public TypedFieldCodec<std::string>
    {
      private:
        Bitset encode();
        Bitset encode(const std::string& wire_value);
        std::string decode(Bitset* bits);
        unsigned size();
        unsigned size(const std::string& wire_value);
        unsigned max_size();
        unsigned min_size();
        void validate();
      private:
        enum { MAX_STRING_LENGTH = 255 };
            
    };


    /// \brief Provides an fixed length byte string encoder.        
    class DefaultBytesCodec : public TypedFieldCodec<std::string>
    {
      private:
        Bitset encode();
        Bitset encode(const std::string& wire_value);
        std::string decode(Bitset* bits);
        unsigned size();
        unsigned size(const std::string& wire_value);
        unsigned max_size();
        unsigned min_size();
        void validate();
    };

    /// \brief Provides an enum encoder. This converts the enumeration to an integer (based on the enumeration <i>index</i> (<b>not</b> its <i>value</i>) and uses DefaultNumericFieldCodec to encode the integer.
    class DefaultEnumCodec
        : public DefaultNumericFieldCodec<int32, const google::protobuf::EnumValueDescriptor*>
    {
      public:
        int32 pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value);
        const google::protobuf::EnumValueDescriptor* post_decode(const int32& wire_value);

      private:
        void validate() { }
            
        double max()
        {
            const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
            return e->value_count()-1;
        }
        double min()
        { return 0; }
    };
        
        
    /// \brief Encodes time of day (second precision) 
    ///
    /// \tparam TimeType A type representing time: See the various specializations of this class for allowed types.
    template<typename TimeType>
        class TimeCodec : public DefaultNumericFieldCodec<int32, TimeType>
    {
        // must use specialization
        BOOST_STATIC_ASSERT(sizeof(TimeType) == 0);
        
      public:
        int32 pre_encode(const TimeType& field_value) {
            
            throw Exception("Not Implemented - Use Specialization");
        }

        TimeType post_decode(const int32& wire_value) {
            throw Exception("Not Implemented - Use Specialization");
        }
 
      private:
        void validate() { }

        double max() { return SECONDS_IN_DAY; }
        double min() { return 0; }
        enum { SECONDS_IN_DAY = 86400 };
    };
    
    template<>
        class TimeCodec<uint64> : public DefaultNumericFieldCodec<int32, uint64>
    {
      public:
        int32 pre_encode(const uint64& time_of_day_microseconds) {
            return (time_of_day_microseconds / 1000000) % SECONDS_IN_DAY;
        }

        uint64 post_decode(const int32& encoded_time) {
            timeval t;
            gettimeofday(&t, 0);
            int64 now = t.tv_sec;
            int64 daystart = now - (now % SECONDS_IN_DAY);
            int64 today_time = now - daystart;

            // If time is more than 12 hours ahead of now, assume it's yesterday.
            if ((encoded_time - today_time) > (SECONDS_IN_DAY/2)) {
                daystart -= SECONDS_IN_DAY;
            } else if ((today_time - encoded_time) > (SECONDS_IN_DAY/2)) {
                daystart += SECONDS_IN_DAY;
            }

            return 1000000 * (daystart + encoded_time);
        }
 
      private:
        void validate() { }

        double max() { return SECONDS_IN_DAY; }
        double min() { return 0; }
        enum { SECONDS_IN_DAY = 86400 };
    };

    template<>
        class TimeCodec<double> : public DefaultNumericFieldCodec<int32, double>
    {
      public:
        int32 pre_encode(const double& time_of_day) {
            return static_cast<int64>(time_of_day) % SECONDS_IN_DAY;
        }

        double post_decode(const int32& encoded_time) {
            timeval t;
            gettimeofday(&t, 0);
            uint64 now = t.tv_sec;
            uint64 daystart = now - (static_cast<int64>(now) % SECONDS_IN_DAY);
            uint64 today_time = now - daystart;

            if ((encoded_time - today_time) > (SECONDS_IN_DAY/2)) {
                daystart -= SECONDS_IN_DAY;
            } else if ((today_time - encoded_time) > (SECONDS_IN_DAY/2)) {
                daystart += SECONDS_IN_DAY;
            }


            return daystart + encoded_time;
        }
 
      private:
        void validate() { }

        double max() { return SECONDS_IN_DAY; }
        double min() { return 0; }
        enum { SECONDS_IN_DAY = 86400 };
    };
        
    /// \brief Placeholder codec that takes no space on the wire (0 bits).
    template<typename T>
        class StaticCodec : public TypedFixedFieldCodec<T>
    {
        Bitset encode(const T&)
        { return Bitset(size()); }

        Bitset encode()
        { return Bitset(size()); }

        T decode(Bitset* bits)
        {
            std::string t = FieldCodecBase::dccl_field_options().static_value();
            return boost::lexical_cast<T>(t);
        }
            
        unsigned size()
        { return 0; }
            
        void validate()
        {
            FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_static_value(), "missing (dccl.field).static_value");
        }
            
    };
        
}


#endif
