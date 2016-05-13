#include "dccl/codec.h"
#include "field_codec_default_message.h"

using dccl::dlog;

//
// DefaultMessageCodec
//

void dccl::v2::DefaultMessageCodec::any_encode(Bitset* bits, const boost::any& wire_value)
{
    if(wire_value.empty())
        *bits = Bitset(min_size());
    else
        *bits = traverse_const_message<Encoder, Bitset>(wire_value);
}
  

 
unsigned dccl::v2::DefaultMessageCodec::any_size(const boost::any& wire_value)
{
    if(wire_value.empty())
        return min_size();
    else
        return traverse_const_message<Size, unsigned>(wire_value);
}


void dccl::v2::DefaultMessageCodec::any_decode(Bitset* bits, boost::any* wire_value)
{
    try
    {
        
        google::protobuf::Message* msg = boost::any_cast<google::protobuf::Message* >(*wire_value);
        
        const google::protobuf::Descriptor* desc = msg->GetDescriptor();
        const google::protobuf::Reflection* refl = msg->GetReflection();
        
        for(int i = 0, n = desc->field_count(); i < n; ++i)
        {
        
            const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

            if(!check_field(field_desc))
                continue;

            
            boost::shared_ptr<FieldCodecBase> codec = find(field_desc);            
            boost::shared_ptr<internal::FromProtoCppTypeBase> helper =
                internal::TypeHelper::find(field_desc);

            if(field_desc->is_repeated())
            {   
                std::vector<boost::any> wire_values;
                if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                {
                    for(unsigned j = 0, m = field_desc->options().GetExtension(dccl::field).max_repeat(); j < m; ++j)
                        wire_values.push_back(refl->AddMessage(msg, field_desc));
                    
                    codec->field_decode_repeated(bits, &wire_values, field_desc);

                    for(int j = 0, m = wire_values.size(); j < m; ++j)
                    {
                        if(wire_values[j].empty()) refl->RemoveLast(msg, field_desc);
                    }
                }
                else
                {
                    // for primitive types
                    codec->field_decode_repeated(bits, &wire_values, field_desc);
                    for(int j = 0, m = wire_values.size(); j < m; ++j)
                        helper->add_value(field_desc, msg, wire_values[j]);
                }
            }
            else
            {
                boost::any wire_value;
                if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                {
                    // allows us to propagate pointers instead of making many copies of entire messages
                    wire_value = refl->MutableMessage(msg, field_desc);
                    codec->field_decode(bits, &wire_value, field_desc);
                    if(wire_value.empty()) refl->ClearField(msg, field_desc);    
                }
                else
                {
                    // for primitive types
                    codec->field_decode(bits, &wire_value, field_desc);
                    helper->set_value(field_desc, msg, wire_value);
                }
            } 
        }

        std::vector< const google::protobuf::FieldDescriptor* > set_fields;
        refl->ListFields(*msg, &set_fields);
        if(set_fields.empty() && this_field()) *wire_value = boost::any();
        else *wire_value = msg;
    }
    catch(boost::bad_any_cast& e)
    {
        throw(Exception("Bad type given to traverse mutable, expecting google::protobuf::Message*, got " + std::string(wire_value->type().name())));
    }

}


unsigned dccl::v2::DefaultMessageCodec::max_size()
{
    unsigned u = 0;
    traverse_descriptor<MaxSize>(&u);
    return u;
}

unsigned dccl::v2::DefaultMessageCodec::min_size()
{
    unsigned u = 0;
    traverse_descriptor<MinSize>(&u);
    return u;
}


void dccl::v2::DefaultMessageCodec::validate()
{
    bool b = false;
    traverse_descriptor<Validate>(&b);
}

std::string dccl::v2::DefaultMessageCodec::info()
{
    std::stringstream ss;
    traverse_descriptor<Info>(&ss);
    return ss.str();
}

bool dccl::v2::DefaultMessageCodec::check_field(const google::protobuf::FieldDescriptor* field)
{
    if(!field)
    {
        return true;
    }
    else
    {
        dccl::DCCLFieldOptions dccl_field_options = field->options().GetExtension(dccl::field);
        if(dccl_field_options.omit()) // omit
        {
            return false;
        }
        else if(internal::MessageStack::current_part() == UNKNOWN) // part not yet explicitly specified
        {
            if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
               find(field)->name() == Codec::default_codec_name()) // default message codec will expand
                return true;
            else if((part() == HEAD && !dccl_field_options.in_head())
                    || (part() == BODY && dccl_field_options.in_head()))
                return false;
            else
                return true;
        }
        else if(internal::MessageStack::current_part() != part()) // part specified and doesn't match
            return false;
        else
            return true;
    }    
}


