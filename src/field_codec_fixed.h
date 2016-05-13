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
