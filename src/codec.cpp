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

#include <algorithm>

#include <dlfcn.h> // for shared library loading

#ifdef HAS_CRYPTOPP
#if CRYPTOPP_PATH_USES_PLUS_SIGN
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#else
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#endif // CRYPTOPP_PATH_USES_PLUS_SIGN
#endif // HAS_CRYPTOPP

#include "dccl/codec.h"
#include "field_codec_default.h"

#include "dccl/protobuf/option_extensions.pb.h"


using dccl::hex_encode;
using dccl::hex_decode;

using namespace dccl;
using namespace dccl::logger;

using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;

const unsigned full_width = 60;


//
// Codec
//

dccl::Codec::Codec(const std::string& dccl_id_codec)
    : id_codec_(dccl_id_codec)
{
    FieldCodecManager::add<DefaultIdentifierCodec>("_default_id_codec");
    // make sure the id codec exists
    id_codec();
    set_default_codecs();
}

void dccl::Codec::set_default_codecs()
{
    // only need to load these once into the static FieldCodecManager
    static bool defaults_loaded = false;

    if(!defaults_loaded)
    {
        using google::protobuf::FieldDescriptor;

        FieldCodecManager::add<DefaultNumericFieldCodec<double> >(default_codec_name());
        FieldCodecManager::add<DefaultNumericFieldCodec<float> >(default_codec_name());
        FieldCodecManager::add<DefaultBoolCodec>(default_codec_name());
        FieldCodecManager::add<DefaultNumericFieldCodec<int32> >(default_codec_name());
        FieldCodecManager::add<DefaultNumericFieldCodec<int64> >(default_codec_name());
        FieldCodecManager::add<DefaultNumericFieldCodec<uint32> >(default_codec_name());
        FieldCodecManager::add<DefaultNumericFieldCodec<uint64> >(default_codec_name());
        FieldCodecManager::add<DefaultStringCodec, FieldDescriptor::TYPE_STRING>(default_codec_name());
        FieldCodecManager::add<DefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(default_codec_name());
        FieldCodecManager::add<DefaultEnumCodec>(default_codec_name());
        FieldCodecManager::add<DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(default_codec_name());
        
        FieldCodecManager::add<TimeCodec<uint64> >("_time");
        FieldCodecManager::add<TimeCodec<int64> >("_time");
        FieldCodecManager::add<TimeCodec<double> >("_time");
        
        FieldCodecManager::add<StaticCodec<std::string> >("_static");
        FieldCodecManager::add<StaticCodec<double> >("_static");
        FieldCodecManager::add<StaticCodec<float> >("_static");
        FieldCodecManager::add<StaticCodec<int32> >("_static");
        FieldCodecManager::add<StaticCodec<int64> >("_static");
        FieldCodecManager::add<StaticCodec<uint32> >("_static");
        FieldCodecManager::add<StaticCodec<uint64> >("_static");

        defaults_loaded = true;
    }
}


void dccl::Codec::encode(std::string* bytes, const google::protobuf::Message& msg, bool header_only /* = false */)
{
    const Descriptor* desc = msg.GetDescriptor();

    dlog.is(DEBUG1, ENCODE) && dlog << "Began encoding message of type: " << desc->full_name() << std::endl;    

    try
    {
        if(!msg.IsInitialized() && !header_only)
            throw(Exception("Message is not properly initialized. All `required` fields must be set."));
        
        if(!id2desc_.count(id(desc)))
            throw(Exception("Message id " + boost::lexical_cast<std::string>(id(desc)) + " has not been validated. Call validate() before encoding this type."));
    
        
        
        boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = TypeHelper::find(desc);

        if(codec)
        {
            //fixed header
            Bitset head_bits;
            id_codec()->field_encode(&head_bits, id(desc), 0);
            
            MessageStack msg_stack;
            msg_stack.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, msg, MessageStack::HEAD);

            std::string body_bytes;
            
            // given header of not even byte size (e.g. 01011), make even byte size (e.g. 00001011)
            unsigned head_byte_size = ceil_bits2bytes(head_bits.size());
            unsigned head_bits_diff = head_byte_size * BITS_IN_BYTE - (head_bits.size());
            head_bits.resize(head_bits.size() + head_bits_diff);
            
            std::string head_bytes = head_bits.to_byte_string();
        
            dlog.is(DEBUG2, ENCODE) && dlog << "Head bytes (bits): " << head_bytes.size() << "(" << head_bits.size() << ")" << std::endl;
            dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (bin): " << head_bits << std::endl;
            dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;        


            if(header_only)
            {
                dlog.is(DEBUG2, ENCODE) && dlog << "as requested, skipping encoding and encrypting body." << std::endl;
            }
            else
            {
                Bitset body_bits;
                codec->base_encode(&body_bits, msg, MessageStack::BODY);
                body_bytes = body_bits.to_byte_string();
                dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (bin): " << body_bits << std::endl;
                dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
                dlog.is(DEBUG2, ENCODE) && dlog << "Body bytes (bits): " <<  body_bytes.size() << "(" << body_bits.size() << ")" <<  std::endl;

                if(!crypto_key_.empty() && !skip_crypto_ids_.count(id(desc)))
                    encrypt(&body_bytes, head_bytes);

                dlog.is(DEBUG3, ENCODE) && dlog << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
            }
            
            dlog.is(DEBUG1, ENCODE) && dlog << "Successfully encoded message of type: " << desc->full_name() << std::endl;

            *bytes += head_bytes + body_bytes;

        }
        else
        {
            throw(Exception("Failed to find (dccl.msg).codec `" + desc->options().GetExtension(dccl::msg).codec() + "`"));
        }
        
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what();

        dlog.is(DEBUG1, ENCODE) && dlog << ss.str() << std::endl;  
        throw(Exception(ss.str()));
    }
}



unsigned dccl::Codec::id(const std::string& bytes)
{
    unsigned id_min_size = 0, id_max_size = 0;
    id_codec()->field_min_size(&id_min_size, 0);
    id_codec()->field_max_size(&id_max_size, 0);
    
    if(bytes.length() < (id_min_size / BITS_IN_BYTE))
        throw(Exception("Bytes passed (hex: " + hex_encode(bytes) + ") is too small to be a valid DCCL message"));
        
    Bitset fixed_header_bits;
    fixed_header_bits.from_byte_string(bytes.substr(0, (size_t)std::ceil(double(id_max_size) / BITS_IN_BYTE)));

    Bitset these_bits(&fixed_header_bits);
    these_bits.get_more_bits(id_min_size);

    boost::any return_value;
    id_codec()->field_decode(&these_bits, &return_value, 0);
    
    return boost::any_cast<uint32>(return_value);
}


void dccl::Codec::decode(std::string* bytes, google::protobuf::Message* msg)
{
    decode(*bytes, msg);
    unsigned last_size = size(*msg);
    bytes->erase(0, last_size);
}

void dccl::Codec::decode(const std::string& bytes, google::protobuf::Message* msg, bool header_only /* = false */)
{
    try
    {
        unsigned this_id = id(bytes);

        dlog.is(DEBUG1, DECODE) && dlog  << "Began decoding message of id: " << this_id << std::endl;
        
        if(!id2desc_.count(this_id))
            throw(Exception("Message id " + boost::lexical_cast<std::string>(this_id) + " has not been validated. Call validate() before decoding this type."));

        const Descriptor* desc = msg->GetDescriptor();
        
        dlog.is(DEBUG1, DECODE) && dlog  << "Type name: " << desc->full_name() << std::endl;
        
        boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = TypeHelper::find(desc);
        
        if(codec)
        {
            unsigned head_size_bits;
            unsigned body_size_bits;
            codec->base_max_size(&head_size_bits, desc, MessageStack::HEAD);
            codec->base_max_size(&body_size_bits, desc, MessageStack::BODY);
            unsigned id_size = 0;
            id_codec()->field_size(&id_size, this_id, 0);            
            head_size_bits += id_size;
        
            unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
            unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
            dlog.is(DEBUG2, DECODE) && dlog  << "Head bytes (bits): " << head_size_bytes << "(" << head_size_bits
                                    << "), max body bytes (bits): " << body_size_bytes << "(" << body_size_bits << ")" <<  std::endl;

            std::string head_bytes = bytes.substr(0, head_size_bytes);
            dlog.is(DEBUG3, DECODE) && dlog  << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
            
            Bitset head_bits;
            head_bits.from_byte_string(head_bytes);    
            dlog.is(DEBUG3, DECODE) && dlog  << "Unencrypted Head (bin): " << head_bits << std::endl;

            // shift off ID bits
            head_bits >>= id_size;

            dlog.is(DEBUG3, DECODE) && dlog  << "Unencrypted Head after ID bits removal (bin): " << head_bits << std::endl;

            MessageStack msg_stack;
            msg_stack.push(msg->GetDescriptor());
            
            codec->base_decode(&head_bits, msg, MessageStack::HEAD);
            dlog.is(DEBUG2, DECODE) && dlog  << "after header decode, message is: " << *msg << std::endl;


            if(header_only)
            {
                dlog.is(DEBUG2, DECODE) && dlog  << "as requested, skipping decrypting and decoding body." << std::endl;
            }
            else
            {
                std::string body_bytes = bytes.substr(head_size_bytes);
                dlog.is(DEBUG3, DECODE) && dlog  << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
                
                if(!crypto_key_.empty() && !skip_crypto_ids_.count(this_id))
                    decrypt(&body_bytes, head_bytes);
                
                dlog.is(DEBUG3, DECODE) && dlog  << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
                
                Bitset body_bits;
                body_bits.from_byte_string(body_bytes);
                dlog.is(DEBUG3, DECODE) && dlog  << "Unencrypted Body (bin): " << body_bits << std::endl;
                
                codec->base_decode(&body_bits, msg, MessageStack::BODY);
                dlog.is(DEBUG2, DECODE) && dlog  << "after header & body decode, message is: " << *msg << std::endl;
            }
        }
        else
        {
            throw(Exception("Failed to find (dccl.msg).codec `" + desc->options().GetExtension(dccl::msg).codec() + "`"));
        }

        dlog.is(DEBUG1, DECODE) && dlog  << "Successfully decoded message of type: " << desc->full_name() << std::endl;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << hex_encode(bytes) <<  " failed to decode. Reason: " << e.what() << std::endl;

        dlog.is(DEBUG1, DECODE) && dlog << ss.str() << std::endl;  
        throw(Exception(ss.str()));
    }    

}

// makes sure we can actual encode / decode a message of this descriptor given the loaded FieldCodecs
// checks all bounds on the message
void dccl::Codec::load(const google::protobuf::Descriptor* desc)
{    
    try
    {
        if(!desc->options().GetExtension(dccl::msg).has_id())
            throw(Exception("Missing message option `(dccl.msg).id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (dccl.msg).id = 3\""));
        if(!desc->options().GetExtension(dccl::msg).has_max_bytes())
            throw(Exception("Missing message option `(dccl.msg).max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (dccl.msg).max_bytes = 32\""));
        
        boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

        unsigned dccl_id = id(desc);
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, MessageStack::HEAD);
        codec->base_max_size(&body_size_bits, desc, MessageStack::BODY);

        unsigned id_bits = 0;
        id_codec()->field_size(&id_bits, dccl_id, 0);
        head_size_bits += id_bits;
        
        const unsigned byte_size = ceil_bits2bytes(head_size_bits) + ceil_bits2bytes(body_size_bits);

        if(byte_size > desc->options().GetExtension(dccl::msg).max_bytes())
            throw(Exception("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));
        
        codec->base_validate(desc, MessageStack::HEAD);
        codec->base_validate(desc, MessageStack::BODY);

        if(id2desc_.count(dccl_id) && desc != id2desc_.find(dccl_id)->second)
            throw(Exception("`dccl id` " + boost::lexical_cast<std::string>(dccl_id) + " is already in use by Message " + id2desc_.find(dccl_id)->second->full_name() + ": " + boost::lexical_cast<std::string>(id2desc_.find(dccl_id)->second)));
        else
            id2desc_.insert(std::make_pair(id(desc), desc));

        dlog.is(DEBUG1) && dlog << "Successfully validated message of type: " << desc->full_name() << std::endl;

    }
    catch(Exception& e)
    {
        try
        {
            info(desc, &dlog);
        }
        catch(Exception& e)
        { }
        
        dlog.is(DEBUG1) && dlog << "Message " << desc->full_name() << ": " << desc << " failed validation. Reason: "
                                << e.what() <<  "\n"
                                << "If possible, information about the Message are printed above. " << std::endl;

        throw;
    }
}

unsigned dccl::Codec::size(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);
    
    unsigned dccl_id = id(desc);
    unsigned head_size_bits;
    codec->base_size(&head_size_bits, msg, MessageStack::HEAD);

    unsigned id_bits = 0;
    id_codec()->field_size(&id_bits, dccl_id, 0);
    head_size_bits += id_bits;
    
    unsigned body_size_bits;
    codec->base_size(&body_size_bits, msg, MessageStack::BODY);

    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    return head_size_bytes + body_size_bytes;
}

void dccl::Codec::info(const google::protobuf::Descriptor* desc, std::ostream* param_os /*= 0 */ ) const
{
    std::ostream* os = (param_os) ? param_os : &dlog;

    if(param_os || dlog.is(INFO))
    {
        try
        {   
            boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

            unsigned config_head_bit_size, body_bit_size;
            codec->base_max_size(&config_head_bit_size, desc, MessageStack::HEAD);
            codec->base_max_size(&body_bit_size, desc, MessageStack::BODY);

            unsigned dccl_id = id(desc);
            unsigned id_bit_size = 0;
            id_codec()->field_size(&id_bit_size, dccl_id, 0);
    
            const unsigned bit_size = id_bit_size + config_head_bit_size + body_bit_size;

        
            const unsigned byte_size = ceil_bits2bytes(config_head_bit_size + id_bit_size) + ceil_bits2bytes(body_bit_size);
        
            const unsigned allowed_byte_size = desc->options().GetExtension(dccl::msg).max_bytes();
            const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;

            std::string guard = std::string((full_width-desc->full_name().size())/2, '=');

            std::string bits_dccl_head_str = "dccl.id head";
            std::string bits_user_head_str = "user head";
            std::string bits_body_str = "body";
            std::string bits_padding_str = "padding to full byte";

            const int bits_width = 40;
            const int spaces = 8;
            std::string indent = std::string(spaces,' ');
            
            *os << guard << " " << desc->full_name() << " " << guard << "\n"
                << "Actual maximum size of message: " << byte_size << " bytes / "
                << byte_size*BITS_IN_BYTE  << " bits\n"
                << indent << bits_dccl_head_str << std::setfill('.') << std::setw(bits_width-bits_dccl_head_str.size()) << id_bit_size << "\n"
                << indent << bits_user_head_str << std::setfill('.') << std::setw(bits_width-bits_user_head_str.size()) << config_head_bit_size << "\n"
                << indent << bits_body_str << std::setfill('.') << std::setw(bits_width-bits_body_str.size()) << body_bit_size << "\n"
                << indent << bits_padding_str << std::setfill('.') << std::setw(bits_width-bits_padding_str.size()) << byte_size * BITS_IN_BYTE - bit_size << "\n"
                << "Allowed maximum size of message: " << allowed_byte_size << " bytes / "
                << allowed_bit_size << " bits\n";

            std::string header_str = "Header";
            std::string header_guard = std::string((full_width-header_str.size())/2, '-');
            *os << header_guard << " " << header_str << " " << header_guard << std::endl;
            *os << bits_dccl_head_str << std::setfill('.') << std::setw(bits_width-bits_dccl_head_str.size()+spaces) << id_bit_size << "\n";
            codec->base_info(os, desc, MessageStack::HEAD);
//            *os << std::string(header_str.size() + 2 + 2*header_guard.size(), '-') << std::endl;
            
            std::string body_str = "Body";
            std::string body_guard = std::string((full_width-body_str.size())/2, '-');
            *os << body_guard << " " << body_str << " " << body_guard << std::endl;
            codec->base_info(os, desc, MessageStack::BODY);
//            *os << std::string(body_str.size() + 2 + 2*body_guard.size(), '-') << std::endl;
                    
//            *os << std::string(desc->full_name().size() + 2 + 2*guard.size(), '=') << std::endl;
        }
        catch(Exception& e)
        {
            dlog.is(DEBUG1) && dlog << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
        }
    }
    
}


void dccl::Codec::encrypt(std::string* s, const std::string& nonce /* message head */)
{
#ifdef HAS_CRYPTOPP
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Encryption encryptor;
    encryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());

    std::string cipher;
    StreamTransformationFilter in(encryptor, new StringSink(cipher));
    in.Put((byte*)s->c_str(), s->size());
    in.MessageEnd();
    *s = cipher;
#endif
}

void dccl::Codec::decrypt(std::string* s, const std::string& nonce)
{
#ifdef HAS_CRYPTOPP
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Decryption decryptor;    
    decryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());
    
    std::string recovered;

    StreamTransformationFilter out(decryptor, new StringSink(recovered));
    out.Put((byte*)s->c_str(), s->size());
    out.MessageEnd();
    *s = recovered;
#endif
}

void dccl::Codec::load_library(const std::string& library_path)
{
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if(handle)
        dl_handles_.push_back(handle);
    load_library(handle);
}

void dccl::Codec::load_library(void* dl_handle)
{
    if(!dl_handle)
        throw(Exception("Null shared library handle passed to load_shared_library_codecs"));
    
    // load any shared library codecs
    void (*dccl_load_ptr)(dccl::Codec*);
    dccl_load_ptr = (void (*)(dccl::Codec*)) dlsym(dl_handle, "dccl3_load");
    if(dccl_load_ptr)
        (*dccl_load_ptr)(this);
}


void dccl::Codec::set_crypto_passphrase(const std::string& passphrase, const std::set<unsigned>& do_not_encrypt_ids_ /*= std::set<unsigned>()*/)
{
    if(!crypto_key_.empty())
        crypto_key_.clear();
    skip_crypto_ids_.clear();

#ifdef HAS_CRYPTOPP
    using namespace CryptoPP;
    
    SHA256 hash;
    StringSource unused(passphrase, true, new HashFilter(hash, new StringSink(crypto_key_)));
    
    dlog.is(DEBUG1) && dlog << "Cryptography enabled with given passphrase" << std::endl;
#else
    dlog.is(DEBUG1) && dlog << "Cryptography disabled because DCCL was compiled without support of Crypto++. Install Crypto++ and recompile to enable cryptography." << std::endl;
#endif

    skip_crypto_ids_ = do_not_encrypt_ids_;
}

void dccl::Codec::info_all(std::ostream* param_os /*= 0 */) const
{
    std::ostream* os = (param_os) ? param_os : &dlog;

    if(param_os || dlog.is(INFO))
    {
        std::string codec_str = "Dynamic Compact Control Language (DCCL) Codec";
        std::string codec_guard = std::string((full_width-codec_str.size())/2, '|');
        *os << codec_guard << " " << codec_str << " " << codec_guard << std::endl;
        
        *os << id2desc_.size() << " messages loaded.\n";            
        *os << "Field sizes are in bits unless otherwise noted." << std::endl;
        
        for(std::map<int32, const google::protobuf::Descriptor*>::const_iterator it = id2desc_.begin(), n = id2desc_.end(); it != n; ++it)
            info(it->second, os);
        
//        *os << std::string(codec_str.size() + 2 + 2*codec_guard.size(), '|') << std::endl;
    }
}
