#include <boost/algorithm/string.hpp> // for replace_all

#include "field_codec.h"
#include "exception.h"
#include "dccl/codec.h"

dccl::MessagePart dccl::FieldCodecBase::part_ =
    dccl::UNKNOWN;

const google::protobuf::Message* dccl::FieldCodecBase::root_message_ = 0;
const google::protobuf::Descriptor* dccl::FieldCodecBase::root_descriptor_ = 0;

using dccl::dlog;
using namespace dccl::logger;

//
// FieldCodecBase public
//
dccl::FieldCodecBase::FieldCodecBase() { }
            
void dccl::FieldCodecBase::base_encode(Bitset* bits,
                                       const google::protobuf::Message& field_value,
                                       MessagePart part)
{
    BaseRAII scoped_globals(part, &field_value);

    // we pass this through the FromProtoCppTypeBase to do dynamic_cast (RTTI) for
    // custom message codecs so that these codecs can be written in the derived class (not google::protobuf::Message)
    field_encode(bits,
                 internal::TypeHelper::find(field_value.GetDescriptor())->get_value(field_value),
                 0);

}

void dccl::FieldCodecBase::field_encode(Bitset* bits,
                                        const boost::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);

    if(field)
        dlog.is(DEBUG2, ENCODE) && dlog << "Starting encode for field: " << field->DebugString() << std::flush;

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);
    
    Bitset new_bits;
    any_encode(&new_bits, wire_value);
    disp_size(field, new_bits, msg_handler.field_.size());
    bits->append(new_bits);
}

void dccl::FieldCodecBase::field_encode_repeated(Bitset* bits,
                                                 const std::vector<boost::any>& field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);
    
    Bitset new_bits;
    any_encode_repeated(&new_bits, wire_values);
    disp_size(field, new_bits, msg_handler.field_.size(), wire_values.size());
    bits->append(new_bits);
}

            
void dccl::FieldCodecBase::base_size(unsigned* bit_size,
                                     const google::protobuf::Message& msg,
                                     MessagePart part)
{
    BaseRAII scoped_globals(part, &msg);

    *bit_size = 0;

    field_size(bit_size, &msg, 0);

}

void dccl::FieldCodecBase::field_size(unsigned* bit_size,
                                      const boost::any& field_value,
                                      const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);

    *bit_size += any_size(wire_value);
}

void dccl::FieldCodecBase::field_size_repeated(unsigned* bit_size,
                                               const std::vector<boost::any>& field_values,
                                               const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);

    *bit_size += any_size_repeated(wire_values);
}




void dccl::FieldCodecBase::base_decode(Bitset* bits,
                                       google::protobuf::Message* field_value,
                                       MessagePart part)
{
    BaseRAII scoped_globals(part, field_value);
    boost::any value(field_value);
    field_decode(bits, &value, 0);
}


void dccl::FieldCodecBase::field_decode(Bitset* bits,
                                        boost::any* field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);
    
    if(!field_value)
        throw(Exception("Decode called with NULL boost::any"));
    else if(!bits)
        throw(Exception("Decode called with NULL Bitset"));    
    
    if(field)
        dlog.is(DEBUG2, DECODE) && dlog << "Starting decode for field: " << field->DebugString() << std::flush;
    
    if(root_message())
        dlog.is(DEBUG3, DECODE) && dlog <<  "Message thus far is: " << root_message()->DebugString() << std::flush;
    
    Bitset these_bits(bits);

    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);    
    
    dlog.is(DEBUG2, DECODE) && dlog  << "... using these bits: " << these_bits << std::endl;

    boost::any wire_value = *field_value;
    
    any_decode(&these_bits, &wire_value);
    
    field_post_decode(wire_value, field_value);  
}

void dccl::FieldCodecBase::field_decode_repeated(Bitset* bits,
                                                 std::vector<boost::any>* field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);
    
    if(!field_values)
        throw(Exception("Decode called with NULL field_values"));
    else if(!bits)
        throw(Exception("Decode called with NULL Bitset"));    
    
    if(field)
        dlog.is(DEBUG2, DECODE) && dlog  << "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits(bits);
    
    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);
    
    dlog.is(DEBUG2, DECODE) && dlog  << "using these " <<
        these_bits.size() << " bits: " << these_bits << std::endl;

    std::vector<boost::any> wire_values = *field_values;
    any_decode_repeated(&these_bits, &wire_values);

    field_values->clear();
    field_post_decode_repeated(wire_values, field_values);
}


void dccl::FieldCodecBase::base_max_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc,
                                         MessagePart part)
{
    BaseRAII scoped_globals(part, desc);
    *bit_size = 0;

    internal::MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Max Size called with NULL Descriptor"));
    
    field_max_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void dccl::FieldCodecBase::field_max_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? max_size_repeated() : max_size();
    else
        *bit_size += max_size();
}


            
void dccl::FieldCodecBase::base_min_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc,
                                         MessagePart part)
{
    BaseRAII scoped_globals(part, desc);

    *bit_size = 0;

    internal::MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Min Size called with NULL Descriptor"));

    field_min_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void dccl::FieldCodecBase::field_min_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)
    
{
    internal::MessageStack msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        *bit_size += min_size();
}

            
void dccl::FieldCodecBase::base_validate(const google::protobuf::Descriptor* desc,
                                         MessagePart part)
{
    BaseRAII scoped_globals(part, desc);

    internal::MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Validate called with NULL Descriptor"));

    bool b = false;
    field_validate(&b, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void dccl::FieldCodecBase::field_validate(bool* b,
                                          const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);

    if(field && dccl_field_options().in_head() && variable_size())
        throw(Exception("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    validate();
}
            
void dccl::FieldCodecBase::base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessagePart part)
{
    BaseRAII scoped_globals(part, desc);

    internal::MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("info called with NULL Descriptor"));

    field_info(os, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void dccl::FieldCodecBase::field_info(std::ostream* os,
                                      const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(field);


    std::stringstream ss;
    int depth = msg_handler.count();
    
    std::string name = ((this_field()) ? boost::lexical_cast<std::string>(this_field()->number()) + ". " + this_field()->name() : this_descriptor()->full_name());
    if(this_field() && this_field()->is_repeated())
        name += "[" + boost::lexical_cast<std::string>(dccl_field_options().max_repeat()) + "]";
    
    
    if(!this_field() || this_field()->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
        depth -= 1;

    const int spaces = 8;
    std::string indent = std::string(spaces*(depth),' ');
    
    const int full_width = 40;
    
    bool is_zero_size = false;

    std::stringstream range;
    if(variable_size())
    {
        unsigned max_sz = 0, min_sz = 0;
        field_max_size(&max_sz, field);
        field_min_size(&min_sz, field);
        if(!max_sz) is_zero_size = true;
        range << min_sz << "-" <<  max_sz;
    }
    else
    {
        unsigned sz = 0;
        field_max_size(&sz, field);
        if(!sz) is_zero_size = true;
        range << sz;
    }

    int width = this_field() ? full_width-name.size() : full_width-name.size()+spaces;
    ss << indent << name <<
        std::setfill('.') << std::setw(std::max(1, width)) << range.str()
       << " {" << (this_field() ? FieldCodecManager::find(this_field(), has_codec_group(), codec_group())->name() : FieldCodecManager::find(root_descriptor_)->name()) << "}";

    
    
//    std::string s = ss.str();
//    boost::replace_all(s, "\n", "\n" + indent);    
//    s = indent + s;

    
    if(!is_zero_size)
        *os << ss.str() << "\n";

    std::string specific_info = info();
    if(!specific_info.empty())
        *os << specific_info;

}

std::string dccl::FieldCodecBase::codec_group(const google::protobuf::Descriptor* desc)
{
    if(desc->options().GetExtension(dccl::msg).has_codec_group())
        return desc->options().GetExtension(dccl::msg).codec_group();
    else
        return Codec::default_codec_name(desc->options().GetExtension(dccl::msg).codec_version());
}


//
// FieldCodecBase protected
//

std::string dccl::FieldCodecBase::info()
{
    return std::string();
}

void dccl::FieldCodecBase::any_encode_repeated(dccl::Bitset* bits, const std::vector<boost::any>& wire_values)
{
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]

    unsigned wire_vector_size = dccl_field_options().max_repeat();

    // for DCCL3 and beyond, add a prefix numeric field giving the vector size (rather than always going to max_repeat
    if(codec_version() > 2)
    {
        wire_vector_size = std::min((int)dccl_field_options().max_repeat(), (int)wire_values.size());    
        Bitset size_bits(repeated_vector_field_size(dccl_field_options().max_repeat()), wire_values.size());
        bits->append(size_bits);
    }    

    for(unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        Bitset new_bits;
        if(i < wire_values.size())
            any_encode(&new_bits, wire_values[i]);
        else
            any_encode(&new_bits, boost::any());
        bits->append(new_bits);
        
    }
}



void dccl::FieldCodecBase::any_decode_repeated(Bitset* repeated_bits, std::vector<boost::any>* wire_values)
{

    unsigned wire_vector_size = dccl_field_options().max_repeat();    
    if(codec_version() > 2)
    {
        Bitset size_bits(repeated_bits);        
        size_bits.get_more_bits(repeated_vector_field_size(dccl_field_options().max_repeat()));

        wire_vector_size = size_bits.to_ulong();
    }

    wire_values->resize(wire_vector_size);
    
    for(unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        Bitset these_bits(repeated_bits);        
        these_bits.get_more_bits(min_size());        
        any_decode(&these_bits, &(*wire_values)[i]);
    }
}

unsigned dccl::FieldCodecBase::any_size_repeated(const std::vector<boost::any>& wire_values)
{
    unsigned out = 0;
    unsigned wire_vector_size = dccl_field_options().max_repeat();

    if(codec_version() > 2)
    {
        wire_vector_size = std::min((int)dccl_field_options().max_repeat(), (int)wire_values.size());    
        out += repeated_vector_field_size(dccl_field_options().max_repeat());
    }    

    for(unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        if(i < wire_values.size())
            out += any_size(wire_values[i]);
        else
            out += any_size(boost::any());
    }    
    return out;
}

unsigned dccl::FieldCodecBase::max_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(Exception("Missing (dccl.field).max_repeat option on `repeated` field: " + this_field()->DebugString()));
    else if(codec_version() > 2)
        return repeated_vector_field_size(dccl_field_options().max_repeat()) + max_size() * dccl_field_options().max_repeat();
    else
        return max_size() * dccl_field_options().max_repeat();
}

unsigned dccl::FieldCodecBase::min_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(Exception("Missing (dccl.field).max_repeat option on `repeated` field " + this_field()->DebugString()));
    else if(codec_version() > 2)
        return repeated_vector_field_size(dccl_field_options().max_repeat());    
    else
        return min_size() * dccl_field_options().max_repeat();
}

void dccl::FieldCodecBase::any_pre_encode_repeated(std::vector<boost::any>* wire_values, const std::vector<boost::any>& field_values)
{
    for(std::vector<boost::any>::const_iterator it = field_values.begin(),
            end = field_values.end(); it != end; ++it)
    {
        boost::any wire_value;
        any_pre_encode(&wire_value, *it);
        wire_values->push_back(wire_value);
    }
    
}
void dccl::FieldCodecBase::any_post_decode_repeated(
    const std::vector<boost::any>& wire_values, std::vector<boost::any>* field_values)
{
    for(std::vector<boost::any>::const_iterator it = wire_values.begin(),
            end = wire_values.end(); it != end; ++it)
    {
        boost::any field_value;
        any_post_decode(*it, &field_value);
        field_values->push_back(field_value);
    }
}


//
// FieldCodecBase private
//

void dccl::FieldCodecBase::disp_size(const google::protobuf::FieldDescriptor* field, const Bitset& new_bits, int depth, int vector_size /* = -1 */)
{
    if(!root_descriptor_)
        return;

    if(dlog.is(INFO, SIZE))
    {   
        std::string name = ((field) ? field->name() : root_descriptor_->full_name());
        if(vector_size >= 0)
            name +=  "[" + boost::lexical_cast<std::string>(vector_size) +  "]";

        
        dlog << std::string(depth, '|') << name << std::setfill('.') << std::setw(40-name.size()-depth) << new_bits.size() << std::endl;
        
        if(!field)
            dlog << std::endl;
        
    }
}
