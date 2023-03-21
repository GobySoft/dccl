// Copyright 2011-2023:
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
#include "field_codec_message_stack.h"
#include "dccl/field_codec.h"

std::vector<const google::protobuf::FieldDescriptor*> dccl::internal::MessageStack::field_;
std::vector<const google::protobuf::Descriptor*> dccl::internal::MessageStack::desc_;
std::vector<dccl::MessagePart> dccl::internal::MessageStack::parts_;

std::vector<dccl::internal::MessageStack::MessageAndField> dccl::internal::MessageStack::messages_;

// MessageStack
//

void dccl::internal::MessageStack::push(const google::protobuf::Descriptor* desc)

{
    desc_.push_back(desc);
    ++descriptors_pushed_;
}

void dccl::internal::MessageStack::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
}

void dccl::internal::MessageStack::push(MessagePart part)
{
    parts_.push_back(part);
    ++parts_pushed_;
}

void dccl::internal::MessageStack::__pop_desc()
{
    if (!desc_.empty())
        desc_.pop_back();
    --descriptors_pushed_;
}

void dccl::internal::MessageStack::__pop_field()
{
    if (!field_.empty())
        field_.pop_back();
    --fields_pushed_;
}

void dccl::internal::MessageStack::__pop_parts()
{
    if (!parts_.empty())
        parts_.pop_back();
    --parts_pushed_;
}

void dccl::internal::MessageStack::__pop_messages()
{
    if (!messages_.empty())
        messages_.pop_back();
    --messages_pushed_;
}

dccl::internal::MessageStack::MessageStack(const google::protobuf::FieldDescriptor* field)
    : descriptors_pushed_(0), fields_pushed_(0), parts_pushed_(0), messages_pushed_(0)
{
    if (field)
    {
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            MessagePart part = UNKNOWN;
            if (field->options().GetExtension(dccl::field).has_in_head())
            {
                // if explicitly set, set part (HEAD or BODY) of message for all children of this message
                part = field->options().GetExtension(dccl::field).in_head() ? HEAD : BODY;
            }
            else
            {
                // use the parent's current part
                part = current_part();
            }
            push(part);
            push(field->message_type());
        }
        push_message(field);
        push(field);
    }
}

void dccl::internal::MessageStack::update_index(const google::protobuf::FieldDescriptor* field,
                                                int index)
{
    push_message(field, index);
}

void dccl::internal::MessageStack::push_message(const google::protobuf::FieldDescriptor* field,
                                                int index)
{
    if (messages_.empty() && dccl::FieldCodecBase::root_message())
    {
        messages_.push_back({dccl::FieldCodecBase::root_message(), nullptr});
        ++messages_pushed_;
    }

    if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
    {
        // replace if the previous push was the same field
        if (!messages_.empty() && field == messages_.back().field && !messages_.empty())
        {
            messages_.pop_back();
            --messages_pushed_;
        }

        // add the new message + field if possible
        if (messages_.size() && messages_.back().msg->GetDescriptor() == field->containing_type())
        {
            const auto* refl = messages_.back().msg->GetReflection();
            if (field->is_repeated())
            {
                if (index >= 0 && index < refl->FieldSize(*messages_.back().msg, field))
                {
                    messages_.push_back(
                        {&refl->GetRepeatedMessage(*messages_.back().msg, field, index), field});
                    ++messages_pushed_;
                }
            }
            else
            {
                messages_.push_back({&refl->GetMessage(*messages_.back().msg, field), field});
                ++messages_pushed_;
            }
        }
    }
}

dccl::internal::MessageStack::~MessageStack()
{
    while (fields_pushed_ > 0) __pop_field();
    while (descriptors_pushed_ > 0) __pop_desc();
    while (parts_pushed_ > 0) __pop_parts();
    while (messages_pushed_ > 0) __pop_messages();
}
