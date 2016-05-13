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
