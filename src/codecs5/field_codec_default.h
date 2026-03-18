// Copyright 2009-2024:
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
// implements FieldCodecBase for all the basic DCCL types for version 5

#ifndef DCCLV5FIELDCODECDEFAULT20240101H
#define DCCLV5FIELDCODECDEFAULT20240101H

#include "../codecs4/field_codec_default.h"

namespace dccl
{
/// DCCL version 5 default field codecs
namespace v5
{
// all these are the same as version 4
template <typename WireType, typename FieldType = WireType>
using DefaultNumericFieldCodec = v4::DefaultNumericFieldCodec<WireType, FieldType>;

using DefaultBoolCodec = v4::DefaultBoolCodec;
using DefaultEnumCodec = v4::DefaultEnumCodec;

using DefaultBytesCodec = v4::DefaultBytesCodec;
using DefaultStringCodec = v4::DefaultStringCodec;

template <typename TimeType> using TimeCodec = v4::TimeCodec<TimeType>;
template <typename T> using StaticCodec = v4::StaticCodec<T>;

} // namespace v5
} // namespace dccl

#endif
