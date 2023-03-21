// Copyright 2009-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Davide Fenucci <davfen@noc.ac.uk>
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
// implements FieldCodecBase for all the basic DCCL types for version 3

#ifndef DCCLV4FIELDCODECDEFAULT20210701H
#define DCCLV4FIELDCODECDEFAULT20210701H

#include "dccl/codecs3/field_codec_default.h"
#include "dccl/codecs3/field_codec_var_bytes.h"

namespace dccl
{
/// DCCL version 4 default field codecs
namespace v4
{
// all these are the same as version 3
template <typename WireType, typename FieldType = WireType>
using DefaultNumericFieldCodec = v3::DefaultNumericFieldCodec<WireType, FieldType>;

using DefaultBoolCodec = v3::DefaultBoolCodec;
using DefaultEnumCodec = v3::DefaultEnumCodec;

using DefaultBytesCodec = v3::VarBytesCodec;
using DefaultStringCodec = v3::VarBytesCodec;
} // namespace v4
} // namespace dccl

#endif
