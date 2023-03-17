// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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
#ifndef DCCLFIELDCODECHELPERS20110825H
#define DCCLFIELDCODECHELPERS20110825H

#include "dccl/common.h"

namespace dccl
{
class FieldCodecBase;
enum MessagePart
{
    HEAD,
    BODY,
    UNKNOWN
};

/// Namespace for objects used internally by DCCL
namespace internal
{

struct MessageStackData
{
    const google::protobuf::Descriptor* top_descriptor() const
    {
        return !desc.empty() ? desc.back() : nullptr;
    }
    const google::protobuf::Message* top_message() const
    {
        return !messages.empty() ? messages.back().msg : nullptr;
    }
    const google::protobuf::FieldDescriptor* top_field() const
    {
        return !field.empty() ? field.back() : nullptr;
    }
    MessagePart current_part() const { return parts.empty() ? UNKNOWN : parts.back(); }

    std::vector<const google::protobuf::Descriptor*> desc;
    std::vector<const google::protobuf::FieldDescriptor*> field;
    std::vector<MessagePart> parts;
    struct MessageAndField
    {
        // latest depth of message
        const google::protobuf::Message* msg;
        // field corresponding to this message (or nullptr for the first)
        const google::protobuf::FieldDescriptor* field{nullptr};
    };
    std::vector<MessageAndField> messages;
};

//RAII handler for the current Message recursion stack
class MessageStack
{
  public:
    MessageStack(MessageStackData& data, const google::protobuf::FieldDescriptor* field = 0);

    ~MessageStack();

    bool first() { return data_.desc.empty(); }
    int count() { return data_.desc.size(); }

    void push(const google::protobuf::Descriptor* desc);
    void push(const google::protobuf::FieldDescriptor* field);
    void push(MessagePart part);

    void update_index(const google::protobuf::FieldDescriptor* field, int index);
    void push_message(const google::protobuf::FieldDescriptor* field, int index = -1);

    std::size_t field_size() { return data_.field.size(); }
    MessagePart current_part() const { return data_.current_part(); }

  private:
    void __pop_desc();
    void __pop_field();
    void __pop_parts();
    void __pop_messages();

    MessageStackData& data_;

    int descriptors_pushed_;
    int fields_pushed_;
    int parts_pushed_;
    int messages_pushed_;
};
} // namespace internal
} // namespace dccl

#endif
