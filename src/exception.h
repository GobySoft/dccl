// Copyright 2009-2019:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
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
#ifndef Exception20100812H
#define Exception20100812H

#include <google/protobuf/descriptor.h>
#include <stdexcept>

namespace dccl
{
/// \brief Exception class for DCCL
class Exception : public std::runtime_error
{
  public:
    Exception(const std::string& s) : std::runtime_error(s) {}
};

/// \brief Exception used to signal null (non-existent) value within field codecs during decode.
class NullValueException : public Exception
{
  public:
    NullValueException() : Exception("NULL Value") {}
};

class OutOfRangeException : public std::out_of_range
{
  public:
    OutOfRangeException(const std::string& s, const google::protobuf::FieldDescriptor* field)
        : std::out_of_range(s), field_(field)
    {
    }

    const google::protobuf::FieldDescriptor* field() const { return field_; }

  private:
    const google::protobuf::FieldDescriptor* field_;
};
} // namespace dccl

#endif
