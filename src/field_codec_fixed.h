// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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
#ifndef DCCLFIELDCODEC20110510H
#define DCCLFIELDCODEC20110510H

#include "field_codec_typed.h"

namespace dccl
{
    /// \brief Base class for static-typed field encoders/decoders that use a fixed number of bits on the wire regardless of the value of the field. Use TypedFieldCodec if your encoder is variable length. See TypedFieldCodec for an explanation of the template parameters (FieldType and WireType).
    ///
    /// \ingroup dccl_field_api
    /// Implements TypedFieldCodec::size(const FieldType& field_value), TypedFieldCodec::max_size and TypedFieldCodec::min_size, and provides a virtual zero-argument function for size()
    template<typename WireType, typename FieldType = WireType>
        class TypedFixedFieldCodec : public TypedFieldCodec<WireType, FieldType>
    {
      protected:
      /// \brief The size of the encoded message in bits. Use TypedFieldCodec if the size depends on the data.
      virtual unsigned size() = 0;
          
      private:
      unsigned size(const WireType& wire_value)
      { return size(); }
          
      unsigned max_size()
      { return size(); }
          
      unsigned min_size()
      { return size(); }          
    };
}

#endif
