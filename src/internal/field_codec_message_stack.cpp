// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
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

//
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
    if(!desc_.empty())
        desc_.pop_back();
}

void dccl::internal::MessageStack::__pop_field()
{
    if(!field_.empty())
        field_.pop_back();
}

void dccl::internal::MessageStack::__pop_parts()
{
    if(!parts_.empty())
        parts_.pop_back();
}


dccl::internal::MessageStack::MessageStack(const google::protobuf::FieldDescriptor* field)
    : descriptors_pushed_(0),
      fields_pushed_(0),
      parts_pushed_(0)
{
    if(field)
    {
        if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            MessagePart part = UNKNOWN;
            if(field->options().GetExtension(dccl::field).has_in_head())
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
        push(field);
    }
    
}

dccl::internal::MessageStack::~MessageStack()
{
    for(int i = 0; i < fields_pushed_; ++i)
        __pop_field();

    for(int i = 0; i < descriptors_pushed_; ++i)
        __pop_desc();

    for(int i = 0; i < parts_pushed_; ++i)
        __pop_parts();    
}
