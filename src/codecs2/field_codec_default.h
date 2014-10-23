// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
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
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>

#include <google/protobuf/descriptor.h>

#include "dccl/protobuf/option_extensions.pb.h"

#include "dccl/codecs2/field_codec_default_message.h"
#include "dccl/field_codec_fixed.h"
#include "dccl/field_codec.h"
#include "dccl/binary.h"

namespace dccl
{
    /// Goby/DCCL version 2 default field codecs
    namespace v2
    {
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

                  validate_numeric_bounds();
              }

              void validate_numeric_bounds()
              {

                  // ensure given max and min fit within WireType ranges
                  FieldCodecBase::require(min() >= boost::numeric::bounds<WireType>::lowest(),
                                          "(dccl.field).min must be >= minimum of this field type.");
                  FieldCodecBase::require(max() <= boost::numeric::bounds<WireType>::highest(),
                                          "(dccl.field).max must be <= maximum of this field type.");

          
                  // ensure value fits into double
                  FieldCodecBase::require((precision() + std::ceil(std::log10(max() - min()))) <= std::numeric_limits<double>::digits10,
                                          "[(dccl.field).max-(dccl.field).min]*10^(dccl.field).precision must fit in a double-precision floating point value. Please increase min, decrease max, or decrease precision.");
          
              }
      
      
              Bitset encode()
              {
                  return Bitset(size());
              }
          
          
              virtual Bitset encode(const WireType& value)
              {
                  // round first, before checking bounds
                  WireType wire_value = dccl::round(value, precision());

                  // check bounds, if out-of-bounds, send as zeros
                  if(wire_value < min() || wire_value > max())
                      return Bitset(size());
          
                  wire_value -= dccl::round((WireType)min(), precision());

                  if (precision() < 0) {
                      wire_value /= (WireType)std::pow(10.0, -precision());
                  } else if (precision() > 0) {
                      wire_value *= (WireType)std::pow(10.0, precision());
                  }

                  dccl::uint64 uint_value = boost::numeric_cast<dccl::uint64>(dccl::round(wire_value, 0));

                  // "presence" value (0)
                  if(!FieldCodecBase::use_required())
                      uint_value += 1;
	  

                  Bitset encoded;
                  encoded.from(uint_value, size());
                  return encoded;
              }
          
              virtual WireType decode(Bitset* bits)
              {
                  // The line below SHOULD BE:
                  // dccl::uint64 t = bits->to<dccl::uint64>();
                  // But GCC3.3 requires an explicit template modifier on the method.
                  // See, e.g., http://gcc.gnu.org/bugzilla/show_bug.cgi?id=10959
                  dccl::uint64 uint_value = (bits->template to<dccl::uint64>)();

                  if(!FieldCodecBase::use_required())
                  {
                      if(!uint_value) throw NullValueException();
                      --uint_value;
                  }
	  
                  WireType wire_value = (WireType)uint_value;

                  if (precision() < 0) {
                      wire_value *= (WireType)std::pow(10.0, -precision());
                  } else if (precision() > 0) {
                      wire_value /= (WireType)std::pow(10.0, precision());
                  }

                  // round values again to properly handle cases where double precision
                  // leads to slightly off values (e.g. 2.099999999 instead of 2.1)
                  wire_value = dccl::round(wire_value + dccl::round((WireType)min(), precision()),
                                           precision());

                  return wire_value;
              }

              unsigned size()
              {
                  // if not required field, leave one value for unspecified (always encoded as 0)
                  const unsigned NULL_VALUE = FieldCodecBase::use_required() ? 0 : 1;
              
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
        
        
        /// \brief Encodes time of day (default: second precision, but can be set with (dccl.field).precision extension) 
        ///
        /// \tparam TimeType A type representing time: See the various specializations of this class for allowed types.

        typedef double time_wire_type;
        template<typename TimeType, int conversion_factor>
            class TimeCodecBase : public DefaultNumericFieldCodec<time_wire_type, TimeType>
        {
          public:
            time_wire_type pre_encode(const TimeType& time_of_day) {
                time_wire_type max_secs = max();
                return std::fmod(time_of_day / static_cast<time_wire_type>(conversion_factor), max_secs);
            }

            TimeType post_decode(const time_wire_type& encoded_time) {

                int64 max_secs = (int64)max();
                timeval t;
                gettimeofday(&t, 0);
                int64 now = t.tv_sec;
                int64 daystart = now - (now % max_secs);
                int64 today_time = now - daystart;

                // If time is more than 12 hours ahead of now, assume it's yesterday.
                if ((encoded_time - today_time) > (max_secs/2)) {
                    daystart -= max_secs;
                } else if ((today_time - encoded_time) > (max_secs/2)) {
                    daystart += max_secs;
                }

                return dccl::round((TimeType)(conversion_factor * (daystart + encoded_time)),
                                   precision() - std::log10((double)conversion_factor));
            }

          private:
            void validate()
            {
                DefaultNumericFieldCodec<time_wire_type, TimeType>::validate_numeric_bounds();
            }

            double max() { 
                return FieldCodecBase::dccl_field_options().num_days() * SECONDS_IN_DAY;
            }

            double min() { return 0; }
            double precision() 
            {
                if(!FieldCodecBase::dccl_field_options().has_precision())
                    return 0; // default to second precision
                else
                {
                    return FieldCodecBase::dccl_field_options().precision() + (double)std::log10((double)conversion_factor);
                }
            
            }
        

          private:
            enum { SECONDS_IN_DAY = 86400 };
        };
    
        template<typename TimeType>
            class TimeCodec : public TimeCodecBase<TimeType, 0>
        { BOOST_STATIC_ASSERT(sizeof(TimeCodec) == 0); };
    
        template<> class TimeCodec<uint64> : public TimeCodecBase<uint64, 1000000> { };
        template<> class TimeCodec<int64> : public TimeCodecBase<int64, 1000000> { };
        template<> class TimeCodec<double> : public TimeCodecBase<double, 1> { };
    
    
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
                return boost::lexical_cast<T>(
                    FieldCodecBase::dccl_field_options().static_value());
            }
            
            unsigned size()
            { return 0; }
            
            void validate()
            {
                FieldCodecBase::require(FieldCodecBase::dccl_field_options().has_static_value(), "missing (dccl.field).static_value");

                std::string t = FieldCodecBase::dccl_field_options().static_value();
                try
                {
                    boost::lexical_cast<T>(t);
                }
                catch(boost::bad_lexical_cast&)
                {
                    FieldCodecBase::require(false, "invalid static_value for this type.");
                }
            }
            
        };
    }
}


#endif
