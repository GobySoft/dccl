#include "dccl/codecs3/field_codec_default.h"

using namespace dccl::logger;

//
// DefaultStringCodec
//

dccl::Bitset dccl::v3::DefaultStringCodec::encode()
{
    return Bitset(min_size());
}

dccl::Bitset dccl::v3::DefaultStringCodec::encode(const std::string& wire_value)
{
    std::string s = wire_value;
    if(s.size() > dccl_field_options().max_length())
    {
        dccl::dlog.is(DEBUG2) && dccl::dlog << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(dccl_field_options().max_length()); 
    }
        
            
    Bitset value_bits;
    value_bits.from_byte_string(s);
    
    Bitset length_bits(min_size(), s.length());

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec value_bits: " << value_bits << std::endl;    

    
    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec length_bits: " << length_bits << std::endl;    
    
    // adds to MSBs
    for(int i = 0, n = value_bits.size(); i < n; ++i)
        length_bits.push_back(value_bits[i]);

    dccl::dlog.is(DEBUG2) && dccl::dlog << "DefaultStringCodec created: " << length_bits << std::endl;
    
    
    return length_bits;
}

std::string dccl::v3::DefaultStringCodec::decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();
    
    if(value_length)
    {
        
        unsigned header_length = min_size();
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "Length of string is = " << value_length << std::endl;
        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits before get_more_bits " << *bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        bits->get_more_bits(value_length*BITS_IN_BYTE);

        
        dccl::dlog.is(DEBUG2) && dccl::dlog << "bits after get_more_bits " << *bits << std::endl;    
        Bitset string_body_bits = *bits;
        string_body_bits >>= header_length;
        string_body_bits.resize(bits->size() - header_length);
    
        return string_body_bits.to_byte_string();
    }
    else
    {
        throw NullValueException();
    }
    
}

unsigned dccl::v3::DefaultStringCodec::size()
{
    return min_size();
}

unsigned dccl::v3::DefaultStringCodec::size(const std::string& wire_value)
{
    return std::min(min_size() + static_cast<unsigned>(wire_value.length()*BITS_IN_BYTE), max_size());
}


unsigned dccl::v3::DefaultStringCodec::max_size()
{
    // string length + actual string
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned dccl::v3::DefaultStringCodec::min_size()
{
    return dccl::ceil_log2(dccl_field_options().max_length()+1);
}


void dccl::v3::DefaultStringCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (dccl.field).max_length");
}
