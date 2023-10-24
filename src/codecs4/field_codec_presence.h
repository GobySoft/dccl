// Copyright 2019:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Kyle Guilbert <kguilbert@aphysci.com>
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
#ifndef FIELD_CODEC4_PRESENCE_H
#define FIELD_CODEC4_PRESENCE_H

#include "../codecs3/field_codec_presence.h"

namespace dccl
{
namespace v4
{
template <typename WrappedType> using PresenceBitCodec = v3::PresenceBitCodec<WrappedType>;
} // namespace v4
} // namespace dccl

#endif
