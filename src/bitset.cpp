#include "bitset.h"
#include "dccl/codec.h"

using namespace dccl::logger;

dccl::Bitset dccl::Bitset::relinquish_bits(size_type num_bits,
                                                           bool final_child)
{
    if(final_child || this->size() < num_bits)
    {
        size_type num_parent_bits = (final_child) ? num_bits : num_bits - this->size();
        if(parent_)
        {
            Bitset parent_bits = parent_->relinquish_bits(num_parent_bits, false);
            append(parent_bits);
        }
    }

    Bitset out;
    if(!final_child)
    {
        for(size_type i = 0; i < num_bits; ++i)
        {
            if(this->empty())
                throw(dccl::Exception("Cannot relinquish_bits - no more bits to give up! Check that all field codecs are always producing (encode) and consuming (decode) the exact same number of bits."));
            
            out.push_back(this->front());
            this->pop_front();
        }
    }
    return out;
}

