#include "field_codec_fixed.h"

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
}
