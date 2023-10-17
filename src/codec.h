// Copyright 2009-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Nathan Knotts <nknotts@gmail.com>
//   philboske <philboske@gmail.com>
//   Chris Murphy <cmurphy@aphysci.com>
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
#ifndef DCCL20091211H
#define DCCL20091211H

#include <map>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <google/protobuf/descriptor.h>

#include <memory>

#include "binary.h"
#include "dynamic_protobuf_manager.h"
#include "exception.h"
#include "field_codec.h"
#include "field_codec_fixed.h"
#include "logger.h"

#include "codecs2/field_codec_default_message.h"
#include "codecs3/field_codec_default_message.h"
#include "dccl/def.h"
#include "field_codec_manager.h"

/// Dynamic Compact Control Language namespace
namespace dccl
{
class FieldCodec;

/// \brief The Dynamic CCL enCODer/DECoder. This is the main class you will use to load, encode and decode DCCL messages. Many users will not need any other DCCL classes than this one.
/// \ingroup dccl_api
class Codec
{
  public:
    /// \brief Instantiate a Codec, optionally with a non-default identifier field codec (loaded via a shared library).
    ///
    /// Normally you will use the default identifier field codec by calling Codec() with no parameters. This will use the DefaultIdentifierCodec to distinguish DCCL message types. However, if you are writing special purpose messages that need to use a different (e.g. more compact) identifier codec, you can load it with FieldCodecManagerLocal::add and then instantiate Codec with that name.
    /// \param dccl_id_codec_name Name passed to FieldCodecManagerLocal::add of a non-standard TypedFieldCodec<uint32> to be used by this Codec to identify message types.
    /// \param library_path Library to load using load_library (this library would typically load the identifier codec referenced in dccl_id_codec).
    Codec(std::string dccl_id_codec_name = default_id_codec_name(),
          const std::string& library_path = "");

    /// \brief Instantiate a Codec with a non-default identifier field codec (loaded directly).
    ///
    /// If you are writing special purpose messages that need to use a different (e.g. more compact) identifier codec, you can instantiate a DCCL Codec that identifier codec using this constructor
    /// \param dccl_id_codec_name Name passed to FieldCodecManagerLocal::add of a non-standard TypedFieldCodec<uint32> to be used by this Codec to identify message types.
    /// \param dccl_id_codec Default instantiation of the IDFieldCodec to use
    /// \tparam IDFieldCodec The type of the TypedFieldCodec<uint32> to be used as the ID codec
    template <class IDFieldCodec,
              typename std::enable_if<std::is_base_of<FieldCodecBase, IDFieldCodec>::value,
                                      int>::type = 0>
    Codec(const std::string& dccl_id_codec_name, const IDFieldCodec& dccl_id_codec) // NOLINT
        : id_codec_(dccl_id_codec_name)
    {
        set_default_codecs();
        manager_.add<IDFieldCodec>(dccl_id_codec_name);
    }

    /// \brief Destructor
    virtual ~Codec();

    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;

    /// \brief Add codecs and/or load messages present in the given shared library handle
    ///
    /// Codecs and messages must be loaded within the shared library using a C function
    /// (declared extern "C") called "dccl3_load" with the signature
    /// void dccl3_load(dccl::Codec* codec)
    void load_library(void* dl_handle);

    /// \brief Remove codecs and/or unload messages present in the given shared library handle
    ///
    /// Codecs and messages must be unloaded within the shared library using a C function
    /// (declared extern "C") called "dccl3_unload" with the signature
    /// void dccl3_unload(dccl::Codec* codec)
    /// Note that codecs must be added before messages that use them are loaded.
    void unload_library(void* dl_handle);

    /// \brief Load any codecs present in the given shared library name.
    ///
    /// The library is opened and then load_library(void* dl_handle) is called. Any libraries
    /// loaded this way will be unloaded when Codec is destructed.
    void load_library(const std::string& library_path);

    /// \brief All messages must be explicited loaded and validated (size checks, option extensions checks, etc.) before they can be encoded/decoded. Use this version of load() when the messages used are static (known at compile time).
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    /// \throw dccl::Exception if message is invalid. Warnings and errors are written to dccl::dlog.
    /// \return Hash of loaded message definition
    template <typename ProtobufMessage> std::size_t load() { return load(ProtobufMessage::descriptor()); }

    /// \brief Unload a given message.
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    template <typename ProtobufMessage> void unload() { unload(ProtobufMessage::descriptor()); }

    void unload_all() { id2desc_.clear(); }

    /// \brief An alterative form for loading and validating messages for message types <i>not</i> known at compile-time ("dynamic").
    ///
    /// \param desc The Google Protobuf "Descriptor" (meta-data) of the message to validate.
    /// \param user_id Custom user_speicified dccl id to identify a message, if user_id is not specified or <0, then the first
    /// dccl_id with the message descriptor corresponding to that of msg will be used
    /// \throw dccl::Exception if message is invalid.
    /// \return Hash of loaded message definition
    std::size_t load(const google::protobuf::Descriptor* desc, int user_id = -1);

    /// \brief An alterative form for unloading messages for message types <i>not</i> known at compile-time ("dynamic").
    ///
    /// \param desc The Google Protobuf "Descriptor" (meta-data) of the message to validate.
    /// \throw dccl::Exception if message is invalid.
    void unload(const google::protobuf::Descriptor* desc);

    /// \brief An alterative form for unloading messages for message types <i>not</i> known at compile-time ("dynamic").
    ///
    /// \param dccl_id Message dccl id
    /// \throw dccl::Exception if message is invalid.
    void unload(size_t dccl_id);

    /// \brief Set a different ID codec name (note that is calls unload_all() so all messages must be reloaded)
    void set_id_codec(const std::string& id_codec_name);
    std::string get_id_codec() { return id_codec_; }

    /// \brief Set a passphrase to be used when encoded messages to encrypt them and to decrypt messages after decoding them.
    ///
    /// Encryption is performed using AES via the opertional Crypto++ library. If this library is not compiled in, no encryption will be performed.
    /// \param passphrase Plain-text passphrase
    /// \param do_not_encrypt_ids_ Optional set of DCCL ids for which to skip encrypting or decrypting
    void
    set_crypto_passphrase(const std::string& passphrase,
                          const std::set<unsigned>& do_not_encrypt_ids_ = std::set<unsigned>());

    /// \brief Set "strict" mode where a dccl::OutOfRangeException will be thrown for encode if the value(s) provided are out of range
    ///
    /// \param mode "true" sets strict mode, "false" disables strict mode
    void set_strict(bool mode) { strict_ = mode; }

    /// \brief Set the number of characters used in programmatic generation of console outputs.
    ///
    /// \param num_chars Character limit for line widths on console outputs
    void set_console_width(unsigned num_chars) { console_width_ = num_chars; }

    //@}

    /// \name Informational Methods.
    ///
    /// Provides various forms of information about the Codec
    //@{

    /// \brief Writes a human readable summary (including field sizes) of the provided DCCL type to the stream provided.
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    /// \param os Pointer to a stream to write this information (if 0, written to dccl::dlog)
    /// \param user_id Custom user_speicified dccl id to identify a message, if user_id is not specified or <0, then the first
    /// dccl_id with the message descriptor corresponding to that of msg will be used
    template <typename ProtobufMessage>
    void info(std::ostream* os = nullptr, int user_id = -1) const
    {
        info(ProtobufMessage::descriptor(), os, user_id);
    }

    /// \brief An alterative form for getting information for messages for message types <i>not</i> known at compile-time ("dynamic").
    ///
    /// \param os Pointer to a stream to write this information (if 0, writes to dccl::dlog)
    /// \param user_id Custom user_speicified dccl id to identify a message, if user_id is not specified or <0, then the first
    /// dccl id with the message descriptor corresponding to that of msg will be used
    void info(const google::protobuf::Descriptor* desc, std::ostream* os = nullptr,
              int user_id = -1) const;

    /// \brief Writes a human readable summary (including field sizes) of all the loaded (validated) DCCL types
    ///
    /// \param os Pointer to a stream to write this information (if 0, writes to dccl::dlog)
    void info_all(std::ostream* os = nullptr) const;

    /// \brief Gives the DCCL id (defined by the custom message option extension "(dccl.msg).id" in the .proto file). This ID is used on the wire to unique identify incoming message types.
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    template <typename ProtobufMessage> unsigned id() const
    {
        return id(ProtobufMessage::descriptor());
    }

    /// \brief Get the DCCL ID of an unknown encoded DCCL message.
    ///
    /// You can use this method along with id() to handle multiple types of known (static) incoming DCCL messages. For example:
    /// \code
    /// unsigned dccl_id = codec.id(bytes);
    /// if(dccl_id == codec.id<MyProtobufType1>())
    /// {
    ///     MyProtobufType1 msg_out1;
    ///     codec.decode(bytes, &msg_out1);
    /// }
    /// else if(dccl_id == codec.id<MyProtobufType2>())
    /// {
    ///     MyProtobufType2 msg_out2;
    ///     codec.decode(bytes, &msg_out2);
    /// }
    /// \endcode
    /// \param bytes encoded message to get the DCCL ID of
    /// \return DCCL ID
    /// \sa test/acomms/dccl8/test.cpp and test/acomms/dccl8/test.proto
    unsigned id(const std::string& bytes) const;

    /// \brief Provides the DCCL ID given a DCCL type.
    template <typename CharIterator> unsigned id(CharIterator begin, CharIterator end) const;

    /// \brief Provides the DCCL ID given a DCCL type.
    unsigned id(const google::protobuf::Descriptor* desc) const
    {
        Bitset id_bits;
        dccl::uint32 hardcoded_id = desc->options().GetExtension(dccl::msg).id();
        // pass the hard coded id, that is, (dccl.msg).id,
        // through encode/decode to allow a custom ID codec (if in use)
        // to always take effect.
        id_codec()->field_encode(&id_bits, hardcoded_id, nullptr);
        std::string id_bytes(id_bits.to_byte_string());
        return id(id_bytes);
    }

    /// \brief Provides a map of all loaded DCCL IDs to the equivalent Protobuf descriptor
    const std::map<int32, const google::protobuf::Descriptor*>& loaded() const { return id2desc_; }

    //@}

    /// \name Codec functions.
    ///
    /// This is where the real work happens.
    //@{

    /// \brief Encodes a DCCL message
    ///
    /// \param bytes Pointer to byte string to store encoded msg
    /// \param msg Message to encode (must already have been validated)
    /// \param header_only If true, only decode the header (do not try to decrypt (if applicable) and decode the message body)
    /// \param user_id Custom user_speicified dccl id to identify a message, if user_id is not specified or <0, then the first
    /// dccl id with the message descriptor corresponding to that of msg will be used
    /// \throw Exception if message cannot be encoded.
    void encode(std::string* bytes, const google::protobuf::Message& msg, bool header_only = false,
                int user_id = -1);

    /// \brief Encodes a DCCL message
    ///
    /// \param bytes Output buffer to store encoded msg
    /// \param max_len Maximum size of output buffer
    /// \param msg Message to encode (must already have been validated)
    /// \param header_only If true, only decode the header (do not try to decrypt (if applicable) and decode the message body)
    /// \param user_id Custom user_speicified dccl id to identify a message, if user_id is not specified or <0, then the first
    /// dccl id with the message descriptor corresponding to that of msg will be used
    /// \throw Exception if message cannot be encoded.
    /// \return size of encoded message
    size_t encode(char* bytes, size_t max_len, const google::protobuf::Message& msg,
                  bool header_only = false, int user_id = -1);

    /// \brief Decode a DCCL message when the type is known at compile time.
    ///
    /// \param begin Iterator to the first byte of encoded message to decode (must already have been validated)
    /// \param end Iterator pointing to the past-the-end character of the message.
    /// \param msg Pointer to any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message). The decoded message will be written here.
    /// \param header_only If true, only decode the header (do not try to decrypt (if applicable) and decode the message body)
    /// \throw Exception if message cannot be decoded.
    /// \return Actual end of decoding, allowing the next message to be decoded starting at this location
    template <typename CharIterator>
    CharIterator decode(CharIterator begin, CharIterator end, google::protobuf::Message* msg,
                        bool header_only = false);

    /// \brief Decode a DCCL message when the type is known at compile time.
    ///
    /// \param bytes encoded message to decode (must already have been validated)
    /// \param msg Pointer to any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message). The decoded message will be written here.
    /// \param header_only If true, only decode the header (do not try to decrypt (if applicable) and decode the message body)
    /// \throw Exception if message cannot be decoded.
    void decode(const std::string& bytes, google::protobuf::Message* msg, bool header_only = false);

    /// \brief Decode a DCCL message when the type is known at compile time.
    ///
    /// \param bytes encoded message to decode (must already have been validated) which will have the used bytes stripped from the front of the encoded message
    /// \param msg Pointer to any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message). The decoded message will be written here.
    /// \throw Exception if message cannot be decoded.
    void decode(std::string* bytes, google::protobuf::Message* msg);

    /// \brief An alterative form for decoding messages for message types <i>not</i> known at compile-time ("dynamic").
    ///
    /// \tparam GoogleProtobufMessagePointer anything that acts like a pointer (has operator*) to a google::protobuf::Message (smart pointers like std::shared_ptr included)
    /// \param bytes the byte string returned by encode
    /// \param header_only If true, only decode the header (do not try to decrypt (if applicable) and decode the message body)
    /// \throw Exception if message cannot be decoded
    /// \return pointer to decoded message (a google::protobuf::Message). You are responsible for deleting the memory used by this pointer, so we recommend using a smart pointer here (e.g. std::shared_ptr or the C++11 equivalent). This message can be examined using the Google Reflection/Descriptor API.
    template <typename GoogleProtobufMessagePointer>
    GoogleProtobufMessagePointer decode(const std::string& bytes, bool header_only = false);

    /// \brief An alterative form for decoding messages for message types <i>not</i> known at compile-time ("dynamic"), where the bytes used are stripped from the front of the encoded message.
    ///
    /// \tparam GoogleProtobufMessagePointer anything that acts like a pointer (has operator*) to a google::protobuf::Message (smart pointers like std::shared_ptr included)
    /// \param bytes encoded message to decode (must already have been validated) which will have the used bytes stripped from the front of the encoded message
    /// \throw Exception if message cannot be decoded
    /// \return pointer to decoded message (a google::protobuf::Message). You are responsible for deleting the memory used by this pointer, so we recommend using a smart pointer here (e.g. std::shared_ptr or the C++11 equivalent). This message can be examined using the Google Reflection/Descriptor API.
    template <typename GoogleProtobufMessagePointer>
    GoogleProtobufMessagePointer decode(std::string* bytes);

    /// \brief Provides the encoded size (in bytes) of msg. This is useful if you need to know the size of a message before encoding it (encoding it is generally much more expensive than calling this method)
    ///
    /// \param msg Google Protobuf message with DCCL extensions for which the encoded size is requested
    /// \param user_id Custom user-specified dccl id to identify a message, if user_id is not specified or <0, then first found
    /// dccl id with the message descriptor corresponding to that of msg will be used
    /// \return Encoded (using DCCL) size in bytes
    unsigned size(const google::protobuf::Message& msg, int user_id = -1);

    /// \brief Provides the encoded maximum size (in bytes) of msg.
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    /// \return Encoded (using DCCL) maximum size in bytes
    template <typename ProtobufMessage> unsigned max_size()
    {
        return max_size(ProtobufMessage::descriptor());
    }

    /// \brief Provides the encoded maximum size (in bytes) of msg.
    unsigned max_size(const google::protobuf::Descriptor* desc) const;

    /// \brief Provides the encoded minimum size (in bytes) of msg.
    ///
    /// \tparam ProtobufMessage Any Google Protobuf Message generated by protoc (i.e. subclass of google::protobuf::Message)
    /// \return Encoded (using DCCL) minimum size in bytes
    template <typename ProtobufMessage> unsigned min_size()
    {
        return min_size(ProtobufMessage::descriptor());
    }

    /// \brief Provides the encoded minimum size (in bytes) of msg.
    unsigned min_size(const google::protobuf::Descriptor* desc) const;

    //@}

    static std::string default_id_codec_name() { return "dccl.default.id"; }

    static std::string default_codec_name(int version = 2)
    {
        switch (version)
        {
            case 2:
                return dccl::DCCLFieldOptions::descriptor()
                    ->FindFieldByName("codec")
                    ->default_value_string();
            default: return "dccl.default" + std::to_string(version);
        }
    }

    FieldCodecManagerLocal& manager() { return manager_; }

  private:
    void encode_internal(const google::protobuf::Message& msg, bool header_only,
                         Bitset& header_bits, Bitset& body_bits, int user_id);
    std::string get_all_error_fields_in_message(const google::protobuf::Message& msg,
                                                uint8_t depth = 1);

    void encrypt(std::string* s, const std::string& nonce);
    void decrypt(std::string* s, const std::string& nonce);

    void set_default_codecs();

    std::shared_ptr<FieldCodecBase> id_codec() const
    {
        return manager_.find(google::protobuf::FieldDescriptor::TYPE_UINT32, id_codec_);
    }

  private:
    // SHA256 hash of the crypto passphrase
    std::string crypto_key_;

    // strict mode setting
    bool strict_{false};

    // console outputting format width
    unsigned console_width_{60};

    // set of DCCL IDs *not* to encrypt
    std::set<unsigned> skip_crypto_ids_;

    // maps `dccl.id`s onto Message Descriptors
    std::map<int32, const google::protobuf::Descriptor*> id2desc_;
    // maps `dccl.id`s onto message Hash
    std::map<int32, std::size_t> id2hash_;
    std::string id_codec_;

    std::vector<void*> dl_handles_;

    std::string build_guard_for_console_output(std::string& base, char guard_char) const;

    FieldCodecManagerLocal manager_;
};

inline std::ostream& operator<<(std::ostream& os, const Codec& codec)
{
    codec.info_all(&os);
    return os;
}
} // namespace dccl

template <typename GoogleProtobufMessagePointer>
GoogleProtobufMessagePointer dccl::Codec::decode(const std::string& bytes,
                                                 bool header_only /* = false */)
{
    unsigned this_id = id(bytes);

    if (!id2desc_.count(this_id))
        throw(Exception("Message id " + std::to_string(this_id) +
                        " has not been loaded. Call load() before decoding this type."));

    // ownership of this object goes to the caller of decode()
    auto msg = dccl::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(
        id2desc_.find(this_id)->second);
    decode(bytes, &(*msg), header_only);
    return msg;
}

template <typename GoogleProtobufMessagePointer>
GoogleProtobufMessagePointer dccl::Codec::decode(std::string* bytes)
{
    unsigned this_id = id(*bytes);

    if (!id2desc_.count(this_id))
        throw(Exception("Message id " + std::to_string(this_id) +
                        " has not been loaded. Call load() before decoding this type."));

    GoogleProtobufMessagePointer msg =
        dccl::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(
            id2desc_.find(this_id)->second);
    std::string::iterator new_begin = decode(bytes->begin(), bytes->end(), &(*msg));
    bytes->erase(bytes->begin(), new_begin);
    return msg;
}

template <typename CharIterator>
unsigned dccl::Codec::id(CharIterator begin, CharIterator end) const
{
    unsigned id_min_size = 0, id_max_size = 0;
    id_codec()->field_min_size(&id_min_size, nullptr);
    id_codec()->field_max_size(&id_max_size, nullptr);

    if (std::distance(begin, end) < (id_min_size / BITS_IN_BYTE))
        throw(Exception("Bytes passed (hex: " + hex_encode(begin, end) +
                        ") is too small to be a valid DCCL message"));

    Bitset fixed_header_bits;
    fixed_header_bits.from_byte_stream(
        begin, begin + (size_t)std::ceil(double(id_max_size) / BITS_IN_BYTE));

    Bitset these_bits(&fixed_header_bits);
    these_bits.get_more_bits(id_min_size);

    dccl::any return_value;
    id_codec()->field_decode(&these_bits, &return_value, nullptr);

    return dccl::any_cast<uint32>(return_value);
}

template <typename CharIterator>
CharIterator dccl::Codec::decode(CharIterator begin, CharIterator end,
                                 google::protobuf::Message* msg, bool header_only /*= false*/)
{
    try
    {
        unsigned this_id = id(begin, end);

        dlog.is(logger::DEBUG1, logger::DECODE) &&
            dlog << "Began decoding message of id: " << this_id << std::endl;

        if (!id2desc_.count(this_id))
            throw(Exception("Message id " + std::to_string(this_id) +
                            " has not been loaded. Call load() before decoding this type."));

        const google::protobuf::Descriptor* desc = msg->GetDescriptor();

        dlog.is(logger::DEBUG1, logger::DECODE) && dlog << "Type name: " << desc->full_name()
                                                        << std::endl;

        std::shared_ptr<FieldCodecBase> codec = manager_.find(desc);
        std::shared_ptr<internal::FromProtoCppTypeBase> helper = manager_.type_helper().find(desc);

        CharIterator actual_end = end;
        if (codec)
        {
            unsigned head_size_bits;
            unsigned body_size_bits;
            codec->base_max_size(&head_size_bits, desc, HEAD);
            codec->base_max_size(&body_size_bits, desc, BODY);
            unsigned id_size = 0;
            id_codec()->field_size(&id_size, this_id, nullptr);
            head_size_bits += id_size;

            unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
            unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);

            dlog.is(logger::DEBUG2, logger::DECODE) &&
                dlog << "Head bytes (bits): " << head_size_bytes << "(" << head_size_bits
                     << "), max body bytes (bits): " << body_size_bytes << "(" << body_size_bits
                     << ")" << std::endl;

            CharIterator head_bytes_end = begin + head_size_bytes;
            dlog.is(logger::DEBUG3, logger::DECODE) &&
                dlog << "Unencrypted Head (hex): " << hex_encode(begin, head_bytes_end)
                     << std::endl;

            Bitset head_bits;
            head_bits.from_byte_stream(begin, head_bytes_end);
            dlog.is(logger::DEBUG3, logger::DECODE) &&
                dlog << "Unencrypted Head (bin): " << head_bits << std::endl;

            // shift off ID bits
            head_bits >>= id_size;

            dlog.is(logger::DEBUG3, logger::DECODE) &&
                dlog << "Unencrypted Head after ID bits removal (bin): " << head_bits << std::endl;

            internal::MessageStack msg_stack(manager_.codec_data().root_message_,
                                             manager_.codec_data().message_data_);
            msg_stack.push(msg->GetDescriptor());

            codec->base_decode(&head_bits, msg, HEAD);
            dlog.is(logger::DEBUG2, logger::DECODE) &&
                dlog << "after header decode, message is: " << *msg << std::endl;

            if (header_only)
            {
                dlog.is(logger::DEBUG2, logger::DECODE) &&
                    dlog << "as requested, skipping decrypting and decoding body." << std::endl;
                actual_end = head_bytes_end;
            }
            else
            {
                dlog.is(logger::DEBUG3, logger::DECODE) &&
                    dlog << "Encrypted Body (hex): " << hex_encode(head_bytes_end, end)
                         << std::endl;

                Bitset body_bits;
                if (!crypto_key_.empty() && !skip_crypto_ids_.count(this_id))
                {
                    std::string head_bytes(begin, head_bytes_end);
                    std::string body_bytes(head_bytes_end, end);
                    decrypt(&body_bytes, head_bytes);
                    dlog.is(logger::DEBUG3, logger::DECODE) &&
                        dlog << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
                    body_bits.from_byte_stream(body_bytes.begin(), body_bytes.end());
                }
                else
                {
                    dlog.is(logger::DEBUG3, logger::DECODE) &&
                        dlog << "Unencrypted Body (hex): " << hex_encode(head_bytes_end, end)
                             << std::endl;
                    body_bits.from_byte_stream(head_bytes_end, end);
                }

                dlog.is(logger::DEBUG3, logger::DECODE) &&
                    dlog << "Unencrypted Body (bin): " << body_bits << std::endl;

                codec->base_decode(&body_bits, msg, BODY);
                dlog.is(logger::DEBUG2, logger::DECODE) &&
                    dlog << "after header & body decode, message is: " << *msg << std::endl;

                actual_end = end - body_bits.size() / BITS_IN_BYTE;
            }
        }
        else
        {
            throw(Exception("Failed to find (dccl.msg).codec `" +
                            desc->options().GetExtension(dccl::msg).codec() + "`"),
                  desc);
        }

        dlog.is(logger::DEBUG1, logger::DECODE) &&
            dlog << "Successfully decoded message of type: " << desc->full_name() << std::endl;
        return actual_end;
    }
    catch (std::exception& e)
    {
        std::stringstream ss;

        ss << "Message " << hex_encode(begin, end) << " failed to decode. Reason: " << e.what()
           << std::endl;

        dlog.is(logger::DEBUG1, logger::DECODE) && dlog << ss.str() << std::endl;
        throw(Exception(ss.str()));
    }
}

#endif
