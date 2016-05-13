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
