#include "field_codec_id.h"

//
// DefaultIdentifierCodec
//

dccl::Bitset dccl::DefaultIdentifierCodec::encode()
{
    return encode(0);
}

dccl::Bitset dccl::DefaultIdentifierCodec::encode(const uint32& id)
{
    if(id <= ONE_BYTE_MAX_ID)
    {
        return(dccl::Bitset(this_size(id), id) << 1);
    }
    else
    {
        dccl::Bitset return_bits(this_size(id), id);
        return_bits <<= 1;
        // set LSB to indicate long header form
        return_bits.set(0, true);

        
        return return_bits;
    }
}

dccl::uint32 dccl::DefaultIdentifierCodec::decode(Bitset* bits)
{
    if(bits->test(0))
    {
        // long header
        // grabs more bits to add to the MSB of `bits`
        bits->get_more_bits((LONG_FORM_ID_BYTES - SHORT_FORM_ID_BYTES)*BITS_IN_BYTE);
        // discard identifier
        *(bits) >>= 1;
        return bits->to_ulong();
    }
    else
    {
        // short header
        *(bits) >>= 1;
        return bits->to_ulong();
    }
}

unsigned dccl::DefaultIdentifierCodec::size()
{
    return this_size(0);
}

unsigned dccl::DefaultIdentifierCodec::size(const uint32& id)
{
    return this_size(id);
}

unsigned dccl::DefaultIdentifierCodec::this_size(const uint32& id)
{
    if(id > TWO_BYTE_MAX_ID)
        throw(Exception("dccl.id provided (" + boost::lexical_cast<std::string>(id) + ") exceeds maximum: " + boost::lexical_cast<std::string>(int(TWO_BYTE_MAX_ID))));
    
    return (id <= ONE_BYTE_MAX_ID) ?
        SHORT_FORM_ID_BYTES*BITS_IN_BYTE :
        LONG_FORM_ID_BYTES*BITS_IN_BYTE;
}


unsigned dccl::DefaultIdentifierCodec::max_size()
{
    return LONG_FORM_ID_BYTES * BITS_IN_BYTE;
}

unsigned dccl::DefaultIdentifierCodec::min_size()
{
    return SHORT_FORM_ID_BYTES * BITS_IN_BYTE;
}
