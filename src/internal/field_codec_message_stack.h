// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
    enum MessagePart { HEAD, BODY, UNKNOWN };

    /// Namespace for objects used internally by DCCL
    namespace internal
    {
        //RAII handler for the current Message recursion stack
        class MessageStack
        {
          public:
            MessageStack(const google::protobuf::FieldDescriptor* field = 0);
                
            ~MessageStack();
            
            bool first() 
            { return desc_.empty(); }
            int count() 
            { return desc_.size(); }

            void push(const google::protobuf::Descriptor* desc);
            void push(const google::protobuf::FieldDescriptor* field);
            void push(MessagePart part);

            static MessagePart current_part() { return parts_.empty() ? UNKNOWN : parts_.back(); }
        
            friend class ::dccl::FieldCodecBase;
          private:
            void __pop_desc();
            void __pop_field();
            void __pop_parts();
                
            static std::vector<const google::protobuf::Descriptor*> desc_;
            static std::vector<const google::protobuf::FieldDescriptor*> field_;
            static std::vector<MessagePart> parts_;
            int descriptors_pushed_;
            int fields_pushed_;
            int parts_pushed_;
        };
    }
}

#endif
