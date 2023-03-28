// Copyright 2021-2022:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Davide Fenucci <davfen@noc.ac.uk>
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
#ifndef DCCLONEOF20210702H
#define DCCLONEOF20210702H

#include "binary.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace dccl
{
/// \brief Checks whether a given field is part to a oneof or not.
/// \param field_desc The field descriptor.
/// \return \c true if the field is part of a oneof, \c false otherwise.
inline bool is_part_of_oneof(const google::protobuf::FieldDescriptor* field_desc)
{
    return field_desc->containing_oneof();
}

/// \brief Returns the index of the containing oneof of the given field, or
/// -1 if the field is not part of a oneof.
/// \param field_desc The field descriptor.
/// \return The index of the containing oneof if the field is part of a oneof, -1 otherwise.
inline int containing_oneof_index(const google::protobuf::FieldDescriptor* field_desc)
{
    if (is_part_of_oneof(field_desc))
        return field_desc->containing_oneof()->index();
    return -1;
}

/// \brief Returns the number of bits needed to represent the oneof cases
/// (including the unset case).
/// \param oneof_desc The oneof descriptor
/// \return The number of bits needed to represent the oneof cases
/// (including the unset case).
inline int oneof_size(const google::protobuf::OneofDescriptor* oneof_desc)
{
    return dccl::ceil_log2(oneof_desc->field_count() + 1);
}
} // namespace dccl

#endif
