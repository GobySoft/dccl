// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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
#include <algorithm>

#include <dlfcn.h> // for shared library loading

#include "dccl/codec.h"

#if DCCL_HAS_CRYPTOPP
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

#include "dccl/codecs2/field_codec_default.h"
#include "dccl/codecs3/field_codec_default.h"
#include "dccl/codecs3/field_codec_var_bytes.h"
#include "dccl/codecs3/field_codec_presence.h"
#include "dccl/field_codec_id.h"

#include "dccl/option_extensions.pb.h"


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

dccl::Codec::Codec(const std::string& dccl_id_codec, const std::string& library_path)
    : strict_(false), id_codec_(dccl_id_codec)
{
    set_default_codecs();
    FieldCodecManager::add<DefaultIdentifierCodec>(default_id_codec_name());

    if(!library_path.empty())
        load_library(library_path);
    // make sure the id codec exists
    id_codec();
}

dccl::Codec::~Codec()
{
    for(std::vector<void *>::iterator it = dl_handles_.begin(),
            n = dl_handles_.end(); it != n; ++it)
    {
        unload_library(*it);
        dlclose(*it);
    }
}


void dccl::Codec::set_default_codecs()
{
    // only need to load these once into the static FieldCodecManager
    static bool defaults_loaded = false;

    if(!defaults_loaded)
    {
        using google::protobuf::FieldDescriptor;

        // version 2
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<double> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<float> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultBoolCodec>(default_codec_name());
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<int32> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<int64> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<uint32> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultNumericFieldCodec<uint64> >(default_codec_name());
        FieldCodecManager::add<v2::DefaultStringCodec, FieldDescriptor::TYPE_STRING>(default_codec_name());
        FieldCodecManager::add<v2::DefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(default_codec_name());
        FieldCodecManager::add<v2::DefaultEnumCodec >(default_codec_name());
        FieldCodecManager::add<v2::DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(default_codec_name());

        FieldCodecManager::add<v2::TimeCodec<uint64> >("dccl.time2");
        FieldCodecManager::add<v2::TimeCodec<int64> >("dccl.time2");
        FieldCodecManager::add<v2::TimeCodec<double> >("dccl.time2");

        FieldCodecManager::add<v2::StaticCodec<std::string> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<double> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<float> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<int32> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<int64> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<uint32> >("dccl.static2");
        FieldCodecManager::add<v2::StaticCodec<uint64> >("dccl.static2");

        // version 3
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<double> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<float> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultBoolCodec>(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<int32> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<int64> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<uint32> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultNumericFieldCodec<uint64> >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultStringCodec, FieldDescriptor::TYPE_STRING>(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultEnumCodec >(default_codec_name(3));
        FieldCodecManager::add<v3::DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(default_codec_name(3));

        // presence bit codec, which encode empty optional fields with a single bit
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<double> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<float> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<int32> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<int64> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<uint32> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<uint64> > >("dccl.presence");
        FieldCodecManager::add<v3::PresenceBitCodec<v3::DefaultEnumCodec > >("dccl.presence");

        // alternative bytes codec that more efficiently encodes variable length bytes fields
        FieldCodecManager::add<v3::VarBytesCodec, FieldDescriptor::TYPE_BYTES>("dccl.var_bytes");

        // for backwards compatibility
        FieldCodecManager::add<v2::TimeCodec<uint64> >("_time");
        FieldCodecManager::add<v2::TimeCodec<int64> >("_time");
        FieldCodecManager::add<v2::TimeCodec<double> >("_time");

        FieldCodecManager::add<v2::StaticCodec<std::string> >("_static");
        FieldCodecManager::add<v2::StaticCodec<double> >("_static");
        FieldCodecManager::add<v2::StaticCodec<float> >("_static");
        FieldCodecManager::add<v2::StaticCodec<int32> >("_static");
        FieldCodecManager::add<v2::StaticCodec<int64> >("_static");
        FieldCodecManager::add<v2::StaticCodec<uint32> >("_static");
        FieldCodecManager::add<v2::StaticCodec<uint64> >("_static");


        defaults_loaded = true;
    }
}

void dccl::Codec::encode_internal(const google::protobuf::Message& msg, bool header_only, Bitset& head_bits, Bitset& body_bits, int user_id)
{
    const Descriptor* desc = msg.GetDescriptor();

    dlog.is(DEBUG1, ENCODE) && dlog << "Began encoding message of type: " << desc->full_name() << std::endl;

    try
    {
        unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
        size_t head_byte_size = 0;

        if(!msg.IsInitialized() && !header_only)
            throw(Exception("Message is not properly initialized. All `required` fields must be set."));

        if(!id2desc_.count(dccl_id))
            throw(Exception("Message id " + boost::lexical_cast<std::string>(dccl_id) + " has not been loaded. Call load() before encoding this type."));



        boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);
        boost::shared_ptr<internal::FromProtoCppTypeBase> helper = internal::TypeHelper::find(desc);

        if(codec)
        {
            //fixed header
            id_codec()->field_encode(&head_bits, dccl_id, 0);

            internal::MessageStack msg_stack;
            msg_stack.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, msg, HEAD, strict_);

            // given header of not even byte size (e.g. 01011), make even byte size (e.g. 00001011)
            head_byte_size = ceil_bits2bytes(head_bits.size());
            head_bits.resize(head_byte_size * BITS_IN_BYTE);

            if(header_only)
            {
                dlog.is(DEBUG2, ENCODE) && dlog << "as requested, skipping encoding and encrypting body." << std::endl;
            }
            else
            {
                codec->base_encode(&body_bits, msg, BODY, strict_);
            }
        }
        else
        {
            throw(Exception("Failed to find (dccl.msg).codec `" + desc->options().GetExtension(dccl::msg).codec() + "`"));
        }

    }
    catch(dccl::OutOfRangeException& e)
    {
        dlog.is(DEBUG1, ENCODE) && dlog << "Message " << desc->full_name() << " failed to encode because a field was out of bounds and strict == true: " << e.what() << std::endl;
        throw;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;

        ss << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what();

        dlog.is(DEBUG1, ENCODE) && dlog << ss.str() << std::endl;
        throw(Exception(ss.str()));
    }
}

size_t dccl::Codec::encode(char* bytes, size_t max_len, const google::protobuf::Message& msg, bool header_only /* = false */, int user_id /* = -1 */)
{
    const Descriptor* desc = msg.GetDescriptor();
    Bitset head_bits;
    Bitset body_bits;
    encode_internal(msg, header_only, head_bits, body_bits, user_id);

    size_t head_byte_size = ceil_bits2bytes(head_bits.size());
    if (max_len < head_byte_size)
    {
        throw std::length_error("max_len must be >= head_byte_size");
    }
    head_bits.to_byte_string(bytes, head_byte_size);

    dlog.is(DEBUG2, ENCODE) && dlog << "Head bytes (bits): " << head_byte_size << "(" << head_bits.size() << ")" << std::endl;
    dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (bin): " << head_bits << std::endl;
    dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (hex): " << hex_encode(bytes, bytes+head_byte_size) << std::endl;

    size_t body_byte_size = 0;
    if (!header_only)
    {
        body_byte_size = ceil_bits2bytes(body_bits.size());
        if (max_len < (head_byte_size + body_byte_size))
        {
            throw std::length_error("max_len must be >= (head_byte_size + body_byte_size)");
        }
        body_bits.to_byte_string(bytes+head_byte_size, max_len-head_byte_size);

        dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (bin): " << body_bits << std::endl;
        dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (hex): " << hex_encode(bytes+head_byte_size, bytes+head_byte_size+body_byte_size) << std::endl;
        dlog.is(DEBUG2, ENCODE) && dlog << "Body bytes (bits): " <<  body_byte_size << "(" << body_bits.size() << ")" <<  std::endl;

        unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
        if(!crypto_key_.empty() && !skip_crypto_ids_.count(dccl_id)) {
            std::string head_bytes(bytes, bytes+head_byte_size);
            std::string body_bytes(bytes+head_byte_size, bytes+head_byte_size+body_byte_size);
            encrypt(&body_bytes, head_bytes);
            std::memcpy(bytes+head_byte_size, body_bytes.data(), body_bytes.size());
        }

        dlog.is(logger::DEBUG3, logger::ENCODE) && dlog << "Encrypted Body (hex): " << hex_encode(bytes+head_byte_size, bytes+head_byte_size+body_byte_size) << std::endl;
    }

    dlog.is(DEBUG1, ENCODE) && dlog << "Successfully encoded message of type: " << desc->full_name() << std::endl;

    return head_byte_size + body_byte_size;
}


void dccl::Codec::encode(std::string* bytes, const google::protobuf::Message& msg, bool header_only /* = false */, int user_id /* = -1 */)
{
    const Descriptor* desc = msg.GetDescriptor();
    Bitset head_bits;
    Bitset body_bits;
    encode_internal(msg, header_only, head_bits, body_bits, user_id);

    std::string head_bytes = head_bits.to_byte_string();

    dlog.is(DEBUG2, ENCODE) && dlog << "Head bytes (bits): " << head_bytes.size() << "(" << head_bits.size() << ")" << std::endl;
    dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (bin): " << head_bits << std::endl;
    dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;

    std::string body_bytes;
    if (!header_only)
    {
        body_bytes = body_bits.to_byte_string();

        dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (bin): " << body_bits << std::endl;
        dlog.is(DEBUG3, ENCODE) && dlog << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
        dlog.is(DEBUG2, ENCODE) && dlog << "Body bytes (bits): " <<  body_bytes.size() << "(" << body_bits.size() << ")" <<  std::endl;

        unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
        if(!crypto_key_.empty() && !skip_crypto_ids_.count(dccl_id))
            encrypt(&body_bytes, head_bytes);

        dlog.is(logger::DEBUG3, logger::ENCODE) && dlog << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
    }

    dlog.is(DEBUG1, ENCODE) && dlog << "Successfully encoded message of type: " << desc->full_name() << std::endl;
    *bytes += head_bytes + body_bytes;
}

unsigned dccl::Codec::id(const std::string& bytes) const
{
    return id(bytes.begin(), bytes.end());
}


void dccl::Codec::decode(std::string* bytes, google::protobuf::Message* msg)
{
    decode(*bytes, msg);
    unsigned last_size = size(*msg);
    bytes->erase(0, last_size);
}

void dccl::Codec::decode(const std::string& bytes, google::protobuf::Message* msg, bool header_only /* = false */)
{
    decode(bytes.begin(), bytes.end(), msg, header_only);
}

// makes sure we can actual encode / decode a message of this descriptor given the loaded FieldCodecs
// checks all bounds on the message
void dccl::Codec::load(const google::protobuf::Descriptor* desc, int user_id /* = -1 */)
{
    try
    {
        if(user_id <0 && !desc->options().GetExtension(dccl::msg).has_id())
            throw(Exception("Missing message option `(dccl.msg).id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (dccl.msg).id = 3\""));
        if(!desc->options().GetExtension(dccl::msg).has_max_bytes())
            throw(Exception("Missing message option `(dccl.msg).max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (dccl.msg).max_bytes = 32\""));

        if(!desc->options().GetExtension(dccl::msg).has_codec_version())
            dlog.is(WARN) && dlog << "** NOTE: No (dccl.msg).codec_version set for DCCL Message '" << desc->full_name() <<  "'. Unless you need backwards compatibility with Goby 2.0 (DCCL2), we highly recommend setting 'option (dccl.msg).codec_version = 3' in the message definition for " << desc->full_name() << " to use the default DCCL3 codecs. If you need compatibility with Goby 2.0, ignore this warning, or set 'option (dccl.msg).codec_version = 2' to remove this warning. **" << std::endl;

        boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

        unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, HEAD);
        codec->base_max_size(&body_size_bits, desc, BODY);

        unsigned id_bits = 0;
        id_codec()->field_size(&id_bits, dccl_id, 0);
        head_size_bits += id_bits;

        const unsigned byte_size = ceil_bits2bytes(head_size_bits) + ceil_bits2bytes(body_size_bits);

        if(byte_size > desc->options().GetExtension(dccl::msg).max_bytes())
            throw(Exception("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));

        codec->base_validate(desc, HEAD);
        codec->base_validate(desc, BODY);

        if(id2desc_.count(dccl_id) && desc != id2desc_.find(dccl_id)->second)
            throw(Exception("`dccl id` " + boost::lexical_cast<std::string>(dccl_id) + " is already in use by Message " + id2desc_.find(dccl_id)->second->full_name() + ": " + boost::lexical_cast<std::string>(id2desc_.find(dccl_id)->second)));
        else
            id2desc_.insert(std::make_pair(dccl_id, desc));

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


void dccl::Codec::unload(const google::protobuf::Descriptor* desc)
{
    unsigned int erased = 0;
    for (std::map<int32, const google::protobuf::Descriptor*>::iterator it = id2desc_.begin(); it != id2desc_.end();)
    {
        if (it->second == desc)
        {
            erased++;
            id2desc_.erase(it++);
        }
        else
        {
            it++;
        }
    }
    if (erased == 0)
    {
        dlog.is(DEBUG1) && dlog << "Message " << desc->full_name() << ": is not loaded. Ignoring unload request." << std::endl;
    }
}


void dccl::Codec::unload(size_t dccl_id)
{
    if(id2desc_.count(dccl_id))
    {
        id2desc_.erase(dccl_id);
    }
    else
    {
        dlog.is(DEBUG1) && dlog << "Message with id " << dccl_id << ": is not loaded. Ignoring unload request." << std::endl;
        return;
    }
}


unsigned dccl::Codec::size(const google::protobuf::Message& msg, int user_id /* = -1 */)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

    unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
    unsigned head_size_bits;
    codec->base_size(&head_size_bits, msg, HEAD);

    unsigned id_bits = 0;
    id_codec()->field_size(&id_bits, dccl_id, 0);
    head_size_bits += id_bits;

    unsigned body_size_bits;
    codec->base_size(&body_size_bits, msg, BODY);

    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    return head_size_bytes + body_size_bytes;
}


unsigned dccl::Codec::max_size(const google::protobuf::Descriptor* desc) const
{
    boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

    unsigned head_size_bits;
    codec->base_max_size(&head_size_bits, desc, HEAD);

    unsigned id_bits = 0;
    id_codec()->field_max_size(&id_bits, 0);
    head_size_bits += id_bits;

    unsigned body_size_bits;
    codec->base_max_size(&body_size_bits, desc, BODY);

    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    return head_size_bytes + body_size_bytes;
}

unsigned dccl::Codec::min_size(const google::protobuf::Descriptor* desc) const
{
    boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

    unsigned head_size_bits;
    codec->base_min_size(&head_size_bits, desc, HEAD);

    unsigned id_bits = 0;
    id_codec()->field_min_size(&id_bits, 0);
    head_size_bits += id_bits;

    unsigned body_size_bits;
    codec->base_min_size(&body_size_bits, desc, BODY);

    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    return head_size_bytes + body_size_bytes;
}



void dccl::Codec::info(const google::protobuf::Descriptor* desc, std::ostream* param_os /*= 0 */, int user_id /* = -1 */) const
{
    std::ostream* os = (param_os) ? param_os : &dlog;

    if(param_os || dlog.is(INFO))
    {
        try
        {
            boost::shared_ptr<FieldCodecBase> codec = FieldCodecManager::find(desc);

            unsigned config_head_bit_size, body_bit_size;
            codec->base_max_size(&config_head_bit_size, desc, HEAD);
            codec->base_max_size(&body_bit_size, desc, BODY);

            unsigned dccl_id = (user_id < 0) ? id(desc) : user_id;
            unsigned id_bit_size = 0;
            id_codec()->field_size(&id_bit_size, dccl_id, 0);

            const unsigned bit_size = id_bit_size + config_head_bit_size + body_bit_size;


            const unsigned byte_size = ceil_bits2bytes(config_head_bit_size + id_bit_size) + ceil_bits2bytes(body_bit_size);

            const unsigned allowed_byte_size = desc->options().GetExtension(dccl::msg).max_bytes();
            const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;

            std::string message_name = boost::lexical_cast<std::string>(dccl_id) + ": " + desc->full_name();
            std::string guard = std::string((full_width-message_name.size())/2, '=');

            std::string bits_dccl_head_str = "dccl.id head";
            std::string bits_user_head_str = "user head";
            std::string bits_body_str = "body";
            std::string bits_padding_str = "padding to full byte";

            const int bits_width = 40;
            const int spaces = 8;
            std::string indent = std::string(spaces,' ');

            *os << guard << " " << message_name << " " << guard << "\n"
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
            *os << bits_dccl_head_str << std::setfill('.') << std::setw(bits_width-bits_dccl_head_str.size()+spaces) << id_bit_size << " {" << id_codec()->name() <<  "}\n";
            codec->base_info(os, desc, HEAD);
//            *os << std::string(header_str.size() + 2 + 2*header_guard.size(), '-') << std::endl;

            std::string body_str = "Body";
            std::string body_guard = std::string((full_width-body_str.size())/2, '-');
            *os << body_guard << " " << body_str << " " << body_guard << std::endl;
            codec->base_info(os, desc, BODY);
//            *os << std::string(body_str.size() + 2 + 2*body_guard.size(), '-') << std::endl;

//            *os << std::string(desc->full_name().size() + 2 + 2*guard.size(), '=') << std::endl;
        }
        catch(Exception& e)
        {
            dlog.is(DEBUG1) && dlog << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
        }

        os->flush();

    }

}


void dccl::Codec::encrypt(std::string* s, const std::string& nonce /* message head */)
{
#if DCCL_HAS_CRYPTOPP
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
#if DCCL_HAS_CRYPTOPP
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
        throw(Exception("Null shared library handle passed to load_library"));

    // load any shared library codecs
    void (*dccl_load_ptr)(dccl::Codec*);
    dccl_load_ptr = (void (*)(dccl::Codec*)) dlsym(dl_handle, "dccl3_load");
    if(dccl_load_ptr)
        (*dccl_load_ptr)(this);
}

void dccl::Codec::unload_library(void* dl_handle)
{
    if(!dl_handle)
        throw(Exception("Null shared library handle passed to unload_library"));

    // unload any shared library codecs
    void (*dccl_unload_ptr)(dccl::Codec*);
    dccl_unload_ptr = (void (*)(dccl::Codec*)) dlsym(dl_handle, "dccl3_unload");
    if(dccl_unload_ptr)
        (*dccl_unload_ptr)(this);
}


void dccl::Codec::set_crypto_passphrase(const std::string& passphrase, const std::set<unsigned>& do_not_encrypt_ids_ /*= std::set<unsigned>()*/)
{
    if(!crypto_key_.empty())
        crypto_key_.clear();
    skip_crypto_ids_.clear();

#if DCCL_HAS_CRYPTOPP
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
            info(it->second, os, it->first);

//        *os << std::string(codec_str.size() + 2 + 2*codec_guard.size(), '|') << std::endl;
    }

}

void dccl::Codec::set_id_codec(const std::string& id_codec_name)
{
    // we must reload messages after setting the id_codec
    unload_all();
    
    id_codec_ = id_codec_name;
    // make sure the id codec exists
    id_codec();
}
