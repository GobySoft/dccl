// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

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

#include "dccl/dccl.h"
#include "dccl_field_codec_default.h"
#include "goby/util/as.h"
#include "dccl/protobuf/option_extensions.pb.h"

//#include "goby/common/header.pb.h"

using goby::common::goby_time;
using goby::util::as;
using goby::util::hex_encode;
using goby::util::hex_decode;

using namespace dccl;
using namespace dccl::logger;

using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;

const std::string dccl::Codec::DEFAULT_CODEC_NAME = "";


//
// Codec
//

dccl::Codec::Codec(const std::string& dccl_id_codec)
    : id_codec_(dccl_id_codec)
{
    DCCLFieldCodecManager::add<DCCLDefaultIdentifierCodec>("_default_id_codec");
    // make sure the id codec exists
    id_codec();
    set_default_codecs();
}

void dccl::Codec::set_default_codecs()
{
    using google::protobuf::FieldDescriptor;
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<double> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<float> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultBoolCodec>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<int32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<int64> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<uint32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<uint64> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultStringCodec, FieldDescriptor::TYPE_STRING>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultEnumCodec>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(DEFAULT_CODEC_NAME);

//    DCCLFieldCodecManager::add<DCCLTimeCodec<std::string> >("_time");
    DCCLFieldCodecManager::add<DCCLTimeCodec<uint64> >("_time");
    DCCLFieldCodecManager::add<DCCLTimeCodec<double> >("_time");

    DCCLFieldCodecManager::add<DCCLStaticCodec<std::string> >("_static"); 
    DCCLFieldCodecManager::add<DCCLModemIdConverterCodec>("_platform<->modem_id");
}


void dccl::Codec::encode(std::string* bytes, const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    dlog.is(DEBUG1) && dlog << "Began encoding message of type: " << desc->full_name() << std::endl;    

    try
    {
        if(!msg.IsInitialized())
            throw(Exception("Message is not properly initialized. All `required` fields must be set."));
        
        if(!id2desc_.count(id(desc)))
            throw(Exception("Message id " +
                                as<std::string>(id(desc))+
                                " has not been loaded. Call load() before encoding this type."));
    
        
        //fixed header
        Bitset head_bits;
        id_codec()->field_encode(&head_bits, id(desc), 0);
        
        Bitset body_bits;
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, msg, MessageHandler::HEAD);
            codec->base_encode(&body_bits, msg, MessageHandler::BODY);
        }
        else
        {
            throw(Exception("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(dccl::msg).codec() + "`"));
        }
        
        // given header of not even byte size (e.g. 01011), make even byte size (e.g. 00001011)
        unsigned head_byte_size = ceil_bits2bytes(head_bits.size());
        unsigned head_bits_diff = head_byte_size * BITS_IN_BYTE - (head_bits.size());
        head_bits.resize(head_bits.size() + head_bits_diff);

        std::string head_bytes = head_bits.to_byte_string();
        std::string body_bytes = body_bits.to_byte_string();

        dlog.is(dccl::logger::DEBUG2) && dlog << "Head bytes (bits): " << head_bytes.size() << "(" << head_bits.size()
                                << "), body bytes (bits): " <<  body_bytes.size() << "(" << body_bits.size() << ")" <<  std::endl;
        dlog.is(dccl::logger::DEBUG3) && dlog << "Unencrypted Head (bin): " << head_bits << std::endl;
        dlog.is(dccl::logger::DEBUG3) && dlog << "Unencrypted Body (bin): " << body_bits << std::endl;
        dlog.is(dccl::logger::DEBUG3) && dlog << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        dlog.is(dccl::logger::DEBUG3) && dlog << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
        
        if(!crypto_key_.empty())
            encrypt(&body_bytes, head_bytes);

        dlog.is(DEBUG3) && dlog << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;


        dlog.is(DEBUG1) && dlog << "Successfully encoded message of type: " << desc->full_name() << std::endl;

        *bytes +=  head_bytes + body_bytes;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what();

        dlog.is(DEBUG1) && dlog << ss.str() << std::endl;  
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
    fixed_header_bits.from_byte_string(bytes.substr(0, std::ceil(double(id_max_size) / BITS_IN_BYTE)));

    Bitset these_bits(&fixed_header_bits);
    these_bits.get_more_bits(id_min_size);

    boost::any return_value;
    id_codec()->field_decode(&these_bits, &return_value, 0);
    
    return boost::any_cast<int32>(return_value);
}


void dccl::Codec::decode(std::string* bytes, google::protobuf::Message* msg)
{
    decode(*bytes, msg);
    unsigned last_size = size(*msg);
    bytes->erase(0, last_size);
}

void dccl::Codec::decode(const std::string& bytes, google::protobuf::Message* msg)
{
    try
    {
        unsigned this_id = id(bytes);

        dlog.is(DEBUG1) && dlog << "Began decoding message of id: " << this_id << std::endl;
        
        if(!id2desc_.count(this_id))
            throw(Exception("Message id " + as<std::string>(this_id) + " has not been loaded. Call load() before decoding this type."));

        const Descriptor* desc = msg->GetDescriptor();
        
        dlog.is(DEBUG1) && dlog << "Type name: " << desc->full_name() << std::endl;
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_size_bits, desc, MessageHandler::BODY);

        unsigned id_bits = 0;
        id_codec()->field_size(&id_bits, this_id, 0);
        head_size_bits += id_bits;
        
        unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
        unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
        dlog.is(DEBUG2) && dlog << "Head bytes (bits): " << head_size_bytes << "(" << head_size_bits
                                << "), max body bytes (bits): " << body_size_bytes << "(" << body_size_bits << ")" <<  std::endl;

        std::string head_bytes = bytes.substr(0, head_size_bytes);
        std::string body_bytes = bytes.substr(head_size_bytes);


        dlog.is(DEBUG3) && dlog << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;


        if(!crypto_key_.empty())
            decrypt(&body_bytes, head_bytes);

        dlog.is(DEBUG3) && dlog << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        dlog.is(DEBUG3) && dlog << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        
        Bitset head_bits, body_bits;
        head_bits.from_byte_string(head_bytes);
        body_bits.from_byte_string(body_bytes);
    
        dlog.is(DEBUG3) && dlog << "Unencrypted Head (bin): " << head_bits << std::endl;
        dlog.is(DEBUG3) && dlog << "Unencrypted Body (bin): " << body_bits << std::endl;

        // shift off ID bits
        head_bits >>= id_bits;

        dlog.is(DEBUG3) && dlog << "Unencrypted Head after ID bits removal (bin): " << head_bits << std::endl;
        
        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg->GetDescriptor());
            
            codec->base_decode(&head_bits, msg, MessageHandler::HEAD);
            dlog.is(DEBUG2) && dlog << "after header decode, message is: " << *msg << std::endl;
            codec->base_decode(&body_bits, msg, MessageHandler::BODY);
            dlog.is(DEBUG2) && dlog << "after header & body decode, message is: " << *msg << std::endl;
        }
        else
        {
            throw(Exception("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(dccl::msg).codec() + "`"));
        }

        dlog.is(DEBUG1) && dlog << "Successfully decoded message of type: " << desc->full_name() << std::endl;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << hex_encode(bytes) <<  " failed to decode. Reason: " << e.what() << std::endl;

        dlog.is(DEBUG1) && dlog << ss.str() << std::endl;  
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
            throw(Exception("Missing message option `(goby.msg).dccl.id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (goby.msg).dccl.id = 3\""));
        if(!desc->options().GetExtension(dccl::msg).has_max_bytes())
            throw(Exception("Missing message option `(goby.msg).dccl.max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (goby.msg).dccl.max_bytes = 32\""));
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);

        unsigned dccl_id = id(desc);
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_size_bits, desc, MessageHandler::BODY);

        unsigned id_bits = 0;
        id_codec()->field_size(&id_bits, dccl_id, 0);
        head_size_bits += id_bits;
        
        const unsigned byte_size = ceil_bits2bytes(head_size_bits) + ceil_bits2bytes(body_size_bits);

        if(byte_size > desc->options().GetExtension(dccl::msg).max_bytes())
            throw(Exception("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));
        
        codec->base_validate(desc, MessageHandler::HEAD);
        codec->base_validate(desc, MessageHandler::BODY);

        if(id2desc_.count(dccl_id) && desc != id2desc_.find(dccl_id)->second)
            throw(Exception("`dccl.id` " + as<std::string>(dccl_id) + " is already in use by Message " + id2desc_.find(dccl_id)->second->full_name() + ": " + as<std::string>(id2desc_.find(dccl_id)->second)));
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

    boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
    
    unsigned dccl_id = id(desc);
    unsigned head_size_bits;
    codec->base_size(&head_size_bits, msg, MessageHandler::HEAD);

    unsigned id_bits = 0;
    id_codec()->field_size(&id_bits, dccl_id, 0);
    head_size_bits += id_bits;
    
    unsigned body_size_bits;
    codec->base_size(&body_size_bits, msg, MessageHandler::BODY);

    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    return head_size_bytes + body_size_bytes;
}

void dccl::Codec::info(const google::protobuf::Descriptor* desc, std::ostream* os) const
{
    try
    {   
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);

        unsigned config_head_bit_size, body_bit_size;
        codec->base_max_size(&config_head_bit_size, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_bit_size, desc, MessageHandler::BODY);

        unsigned dccl_id = id(desc);
        unsigned id_bit_size = 0;
        id_codec()->field_size(&id_bit_size, dccl_id, 0);
    
        const unsigned bit_size = id_bit_size + config_head_bit_size + body_bit_size;

        
        const unsigned byte_size = ceil_bits2bytes(config_head_bit_size + id_bit_size) + ceil_bits2bytes(body_bit_size);
        
        const unsigned allowed_byte_size = desc->options().GetExtension(dccl::msg).max_bytes();
        const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;
        
        *os << "= Begin " << desc->full_name() << " =\n"
            << "Actual maximum size of message: " << byte_size << " bytes / "
            << byte_size*BITS_IN_BYTE  << " bits [dccl.id head: " << id_bit_size
            << ", user head: " << config_head_bit_size << ", body: "
            << body_bit_size << ", padding: " << byte_size * BITS_IN_BYTE - bit_size << "]\n"
            << "Allowed maximum size of message: " << allowed_byte_size << " bytes / "
            << allowed_bit_size << " bits\n";

        *os << "== Begin Header ==" << std::endl;
        codec->base_info(os, desc, MessageHandler::HEAD);
        *os << "== End Header ==" << std::endl;
        *os << "== Begin Body ==" << std::endl;
        codec->base_info(os, desc, MessageHandler::BODY);
        *os << "== End Body ==" << std::endl;
        
        *os << "= End " << desc->full_name() << " =" << std::endl;
    }
    catch(Exception& e)
    {
        dlog.is(DEBUG1) && dlog << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
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

void dccl::Codec::load_library(void* dl_handle)
{
    if(!dl_handle)
        throw(Exception("Null shared library handle passed to load_shared_library_codecs"));
    
    // load any shared library codecs
    void (*dccl_load_ptr)(dccl::Codec*);
    dccl_load_ptr = (void (*)(dccl::Codec*)) dlsym(dl_handle, "goby_dccl_load");
    if(dccl_load_ptr)
        (*dccl_load_ptr)(this);
}



void dccl::Codec::set_crypto_passphrase(const std::string& passphrase)
{
    if(!crypto_key_.empty())
        crypto_key_.clear();
#ifdef HAS_CRYPTOPP
    using namespace CryptoPP;
    
    SHA256 hash;
    StringSource unused(passphrase, true, new HashFilter(hash, new StringSink(crypto_key_)));
    
    dlog.is(DEBUG1) && dlog << "Cryptography enabled with given passphrase" << std::endl;
#else
    dlog.is(DEBUG1) && dlog << "Cryptography disabled because Goby was compiled without support of Crypto++. Install Crypto++ and recompile to enable cryptography." << std::endl;
#endif

}

void dccl::Codec::info_all(std::ostream* os) const
{
    *os << "=== Begin Codec ===" << "\n";
    *os << id2desc_.size() << " messages loaded.\n";            
            
    for(std::map<int32, const google::protobuf::Descriptor*>::const_iterator it = id2desc_.begin(), n = id2desc_.end(); it != n; ++it)
        info(it->second, os);
                
    *os << "=== End Codec ===";
}
