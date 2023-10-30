// Copyright 2009-2023:
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
#ifndef Exception20100812H
#define Exception20100812H

#include <google/protobuf/descriptor.h>
#include <stdexcept>

namespace dccl
{
inline std::string exception_string(const std::string& in, const google::protobuf::Descriptor* desc,
                                    const google::protobuf::FieldDescriptor* field)
{
    std::string out;
    if (desc)
        out += std::string("Message: ") + desc->full_name() + ": ";
    if (field)
        out += std::string("Field: ") + field->full_name() + ": ";

    out += in;
    return out;
}

/// \brief Exception class for DCCL
class Exception : public std::runtime_error
{
  public:
    Exception(const std::string& s, const google::protobuf::Descriptor* desc = nullptr)
        : std::runtime_error(exception_string(s, desc, nullptr)), desc_(desc)
    {
    }

    const google::protobuf::Descriptor* desc() const { return desc_; }

  private:
    const google::protobuf::Descriptor* desc_;
};

/// \brief Exception used to signal null (non-existent) value within field codecs during decode.
class NullValueException : public Exception
{
  public:
    NullValueException() : Exception(exception_string("NULL value", nullptr, nullptr)) {}
};

class OutOfRangeException : public std::out_of_range
{
  public:
    OutOfRangeException(const std::string& s, const google::protobuf::FieldDescriptor* field,
                        const google::protobuf::Descriptor* desc = nullptr)
        : std::out_of_range(exception_string(s, desc, field)), field_(field), desc_(desc)
    {
    }

    const google::protobuf::FieldDescriptor* field() const { return field_; }
    const google::protobuf::Descriptor* desc() const { return desc_; }

  private:
    const google::protobuf::FieldDescriptor* field_;
    const google::protobuf::Descriptor* desc_;
};
} // namespace dccl

#endif
