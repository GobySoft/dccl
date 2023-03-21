// Copyright 2014-2023:
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
#ifndef TypeHelper20110405H
#define TypeHelper20110405H

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/version.hpp>

#include "protobuf_cpp_type_helpers.h"

namespace dccl
{
class FieldCodecManagerLocal;

namespace internal
{
/// \brief Provides FromProtoTypeBase and FromProtoCppTypeBase type identification helper classes for various representations of the underlying field.
class TypeHelper
{
  public:
    TypeHelper() { initialize(); }
    ~TypeHelper() {}

    boost::shared_ptr<FromProtoTypeBase> find(google::protobuf::FieldDescriptor::Type type) const;
    boost::shared_ptr<FromProtoCppTypeBase>
    find(const google::protobuf::FieldDescriptor* field) const
    {
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            return find(field->message_type());
        else
            return find(field->cpp_type());
    }

    boost::shared_ptr<FromProtoCppTypeBase> find(const google::protobuf::Descriptor* desc) const
    {
        return find(google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE, desc->full_name());
    }

    boost::shared_ptr<FromProtoCppTypeBase> find(google::protobuf::FieldDescriptor::CppType cpptype,
                                                 const std::string& type_name = "") const;

  private:
    friend class ::dccl::FieldCodecManagerLocal;
    template <typename ProtobufMessage> void add()
    {
        custom_message_map_.insert(std::make_pair(
            ProtobufMessage::descriptor()->full_name(),
            boost::shared_ptr<FromProtoCppTypeBase>(new FromProtoCustomMessage<ProtobufMessage>)));
    }
    template <typename ProtobufMessage> void remove()
    {
        custom_message_map_.erase(ProtobufMessage::descriptor()->full_name());
    }
    void reset()
    {
        type_map_.clear();
        cpptype_map_.clear();
        custom_message_map_.clear();
    }

    void initialize();

  public:
    typedef std::map<google::protobuf::FieldDescriptor::Type, boost::shared_ptr<FromProtoTypeBase>>
        TypeMap;
    TypeMap type_map_;

    typedef std::map<google::protobuf::FieldDescriptor::CppType,
                     boost::shared_ptr<FromProtoCppTypeBase>>
        CppTypeMap;
    CppTypeMap cpptype_map_;

    typedef std::map<std::string, boost::shared_ptr<FromProtoCppTypeBase>> CustomMessageMap;
    CustomMessageMap custom_message_map_;
};
} // namespace internal
} // namespace dccl

#endif
