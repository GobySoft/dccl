// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
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
#include "field_codec.h"

std::vector<const google::protobuf::FieldDescriptor*> dccl::MessageStack::field_;
std::vector<const google::protobuf::Descriptor*> dccl::MessageStack::desc_;
dccl::MessageStack::MessagePart dccl::MessageStack::current_part_ = dccl::MessageStack::UNKNOWN;

//
// MessageStack
//

void dccl::MessageStack::push(const google::protobuf::Descriptor* desc)
 
{
    desc_.push_back(desc);
    ++descriptors_pushed_;
}

void dccl::MessageStack::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
}


void dccl::MessageStack::__pop_desc()
{
    if(!desc_.empty())
        desc_.pop_back();
}

void dccl::MessageStack::__pop_field()
{
    if(!field_.empty())
        field_.pop_back();
}

dccl::MessageStack::MessageStack(const google::protobuf::FieldDescriptor* field)
    : descriptors_pushed_(0),
      fields_pushed_(0)
{
    if(field)
    {
        if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            // if explicitly set, set part (HEAD or BODY) of message for all children of this message
            if(field->options().GetExtension(dccl::field).has_in_head())
                current_part_ = field->options().GetExtension(dccl::field).in_head() ? HEAD : BODY;
            
            push(field->message_type());
        }
        push(field);
    }
    
}

