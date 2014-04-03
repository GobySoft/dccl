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
