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



// implements FieldCodecBase for numeric values, using a presence bit for optional fields

#ifndef DCCLFIELDCODECPRESENCEBIT20140109H
#define DCCLFIELDCODECPRESENCEBIT20140109H

#include "field_codec_typed.h"

namespace dccl
{
template<typename WireType, typename FieldType = WireType>
    class PresenceBitNumericFieldCodec : public dccl::RepeatedTypedFieldCodec<WireType, FieldType>
    {
      private:
      virtual double max()
      { return dccl::FieldCodecBase::dccl_field_options().max(); }

      virtual double min()
      { return dccl::FieldCodecBase::dccl_field_options().min(); }

      virtual double precision()
      { return dccl::FieldCodecBase::dccl_field_options().precision(); }
            
      virtual void validate()
      {
          
          dccl::FieldCodecBase::require(dccl::FieldCodecBase::dccl_field_options().has_min(), "missing (dccl.field).min");
          dccl::FieldCodecBase::require(dccl::FieldCodecBase::dccl_field_options().has_max(), "missing (dccl.field).max");

          if(dccl::FieldCodecBase::this_field()->is_repeated())
              dccl::FieldCodecBase::require(dccl::FieldCodecBase::dccl_field_options().has_max_repeat(), "(dccl.field).max_repeat must be set for repeated fields");

          // ensure given max and min fit within WireType ranges
          dccl::FieldCodecBase::require(min() >= boost::numeric::bounds<WireType>::lowest(),
                                      "(dccl.field).min must be >= minimum of this field type.");
          dccl::FieldCodecBase::require(max() <= boost::numeric::bounds<WireType>::highest(),
                                      "(dccl.field).max must be <= maximum of this field type.");
      }

      
      dccl::Bitset encode_repeated(const std::vector<WireType>& wire_values)
      {
          dccl::Bitset all_bits;
          for(int i = 0, n = min(wire_values.size(), dccl::FieldCodecBase::dccl_field_options().max_repeat()); i < n; ++i)
          {
              WireType wire_value = wire_values[i];

              // pre-round before checking bounds
              if(precision() >= 0)
                  wire_value = dccl::round(wire_value, precision());
                  
                  
              if(wire_value < min() || wire_value > max())
                  break;
              
              wire_value -= min();
              wire_value *= std::pow(10.0, precision());

              wire_value = dccl::round(wire_value, 0);
              const unsigned PRESENCE_BIT = 1; 
              dccl::Bitset bits(single_present_field_size() - PRESENCE_BIT,
                                        boost::numeric_cast<dccl::uint64>(wire_value));
              
              bits.push_front(true);
              all_bits.append(bits);
          }
          // EOF symbol
          all_bits.push_back(false);
          return all_bits;
      }
      
      WireType decode(dccl::Bitset* bits)
      {
          std::vector<WireType> return_vec = decode_repeated(bits);
          if(return_vec.empty())
              throw dccl::NullValueException();
          else
              return return_vec.at(0);
      }

      std::vector<WireType> decode_repeated(dccl::Bitset* bits)
      {
          std::vector<WireType> return_vec;
          while(bits->to_ulong())
          {
              const unsigned PRESENCE_BIT = 1; 
              bits->get_more_bits(single_present_field_size()-PRESENCE_BIT);
              (*bits) >>= 1;
              dccl::uint64 t = (bits->template to<dccl::uint64>)();
              WireType return_value = dccl::round(
                  t / (std::pow(10.0, precision())) + min(), precision());
              
              return_vec.push_back(return_value);
              bits->resize(0);
              bits->get_more_bits(1);
          }
          return return_vec;
      }

      unsigned size_repeated(const std::vector<WireType>& wire_values)
      {
          return single_present_field_size()*wire_values.size() + min_size_repeated();
      }

      unsigned max_size_repeated()
      {
          return single_present_field_size()*dccl::FieldCodecBase::dccl_field_options().max_repeat() + min_size_repeated();
      }

      unsigned min_size_repeated()
      {
          return 1;
      }
      
      int min(int a, int b)
      { return std::min(a, b); }      

      unsigned single_present_field_size() 
      { 
          const unsigned PRESENCE_BIT = 1; 
          return dccl::ceil_log2((max()-min())*std::pow(10.0, precision())+1) + PRESENCE_BIT; 
      }
    };

    class PresenceBitEnumFieldCodec
        : public PresenceBitNumericFieldCodec<dccl::int32, const google::protobuf::EnumValueDescriptor*>
    {
      public:
        dccl::int32 pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
        {
            return field_value->index();
        }
    
        const google::protobuf::EnumValueDescriptor* post_decode(const dccl::int32& wire_value)
        {
            const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
        
            if(wire_value < e->value_count())
            {
                const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);
                return return_value;
            }
            else
                throw(dccl::NullValueException());
        
        }
    
    
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

}



#endif
