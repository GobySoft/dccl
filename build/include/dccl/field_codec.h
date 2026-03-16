// Copyright 2011-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
#ifndef DCCLFIELDCODEC20110322H
#define DCCLFIELDCODEC20110322H

#include <map>
#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>

#include "any.h"
#include "binary.h"
#include "common.h"
#include "dynamic_conditions.h"
#include "exception.h"
#include "internal/field_codec_message_stack.h"
#include "internal/type_helper.h"
#include "oneof.h"
#include "option_extensions.pb.h"

namespace dccl
{
class Codec;
namespace internal
{
class MessageStack;
}

/// \brief Provides a base class for defining DCCL field encoders / decoders. Most users who wish to define custom encoders/decoders will use the RepeatedTypedFieldCodec, TypedFieldCodec or its children (e.g. TypedFixedFieldCodec) instead of directly inheriting from this class.
class FieldCodecBase
{
  public:
    /// \name Constructor, Destructor
    //@{

    FieldCodecBase();
    virtual ~FieldCodecBase() = default;
    //@}

    /// \name Information Methods
    //@{
    /// \brief the name of the codec used to identifier it in the .proto custom option extension
    std::string name() const { return name_; }
    /// \brief the type exposed to the user in the original and decoded Protobuf messages
    ///
    /// \return the Google Protobuf message type. See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.descriptor.html#FieldDescriptor.Type.details
    google::protobuf::FieldDescriptor::Type field_type() const { return field_type_; }
    /// \brief the C++ type used "on the wire". This is the type visible <i>after</i> pre_encode and <i>before</i> post_decode functions are called.
    ///
    /// The wire type allows codecs to make type changes (e.g. from string to integer) before reusing another codec that knows how to encode that wire type (e.g. DefaultNumericFieldCodec)
    /// \return the C++ type used to encode and decode. See http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.descriptor.html#FieldDescriptor.CppType.details
    google::protobuf::FieldDescriptor::CppType wire_type() const { return wire_type_; }

    /// \brief Returns the FieldDescriptor (field schema  meta-data) for this field
    ///
    /// \return FieldDescriptor for the current field or 0 if this codec is encoding the base message.
    const google::protobuf::FieldDescriptor* this_field() const;

    /// \brief Returns the Descriptor (message schema meta-data) for the immediate parent Message
    ///
    /// for:
    /// \code
    /// message Foo
    /// {
    ///    int32 bar = 1;
    ///    FooBar baz = 2;
    /// }
    /// \endcode
    /// returns Descriptor for Foo if this_field() == 0
    /// returns Descriptor for Foo if this_field() == FieldDescriptor for bar
    /// returns Descriptor for FooBar if this_field() == FieldDescriptor for baz
    const google::protobuf::Descriptor* this_descriptor() const;

    const google::protobuf::Message* this_message();

    // currently encoded or (partially) decoded root message
    const google::protobuf::Message* root_message();

    const google::protobuf::Descriptor* root_descriptor() const;

    internal::MessageStackData& message_data();

    const internal::MessageStackData& message_data() const;

    bool has_codec_group();

    static std::string codec_group(const google::protobuf::Descriptor* desc);

    std::string codec_group();

    int codec_version();

    /// \brief the part of the message currently being encoded (head or body)
    MessagePart part();

    bool strict();

    /// \brief Force the codec to always use the "required" field encoding, regardless of the FieldDescriptor setting. Useful when wrapping this codec in another that handles optional and repeated fields
    void set_force_use_required(bool force_required = true) { force_required_ = force_required; }

    //@}

    /// \name Base message functions
    ///
    /// These are called typically by Codec to start processing a new message. In this example "Foo" is a base message:
    /// \code
    /// message Foo
    /// {
    ///    int32 bar = 1;
    ///    FooBar baz = 2;
    /// }
    /// \endcode
    //@{

    /// \brief Encode this part (body or head) of the base message
    ///
    /// \param bits pointer to a Bitset where all bits will be pushed on to the most significant end.
    /// \param msg DCCL Message to encode
    /// \param part Part of the message to encode
    void base_encode(Bitset* bits, const google::protobuf::Message& msg, MessagePart part,
                     bool strict);

    /// \brief Calculate the size (in bits) of a part of the base message when it is encoded
    ///
    /// \param bit_size Pointer to unsigned integer to store the result.
    /// \param msg the DCCL Message of which to calculate the size
    /// \param part part of the Message to calculate the size of
    void base_size(unsigned* bit_size, const google::protobuf::Message& msg, MessagePart part);

    /// \brief Decode part of a message
    ///
    /// \param bits Pointer to a Bitset containing bits to decode. The least significant bits will be consumed first. Any bits not consumed will remain in `bits` after this method returns.
    /// \param msg DCCL Message to <i>merge</i> the decoded result into.
    /// \param part part of the Message to decode
    void base_decode(Bitset* bits, google::protobuf::Message* msg, MessagePart part);

    /// \brief Calculate the maximum size of a message given its Descriptor alone (no data)
    ///
    /// \param bit_size Pointer to unsigned integer to store calculated maximum size in bits.
    /// \param desc Descriptor to calculate the maximum size of. Use google::protobuf::Message::GetDescriptor() or MyProtobufType::descriptor() to get this object.
    /// \param part part of the Message
    void base_max_size(unsigned* bit_size, const google::protobuf::Descriptor* desc,
                       MessagePart part);

    /// \brief Calculate the minimum size of a message given its Descriptor alone (no data)
    ///
    /// \param bit_size Pointer to unsigned integer to store calculated minimum size in bits.
    /// \param desc Descriptor to calculate the minimum size of. Use google::protobuf::Message::GetDescriptor() or MyProtobufType::descriptor() to get this object.
    /// \param part part of the Message
    void base_min_size(unsigned* bit_size, const google::protobuf::Descriptor* desc,
                       MessagePart part);

    /// \brief Validate this part of the message to make sure all required extensions are set.
    ///
    /// \param desc Descriptor to validate. Use google::protobuf::Message::GetDescriptor() or MyProtobufType::descriptor() to get this object.
    /// \param part part of the Message
    void base_validate(const google::protobuf::Descriptor* desc, MessagePart part);

    /// \brief Get human readable information (size of fields, etc.) about this part of the DCCL message
    ///
    /// \param os Pointer to stream to store this information
    /// \param desc Descriptor to get information on. Use google::protobuf::Message::GetDescriptor() or MyProtobufType::descriptor() to get this object.
    /// \param part the part of the Message to act on.
    void base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessagePart part);

    /// \brief Provide a hash of the DCCL message definition to detect changes in the DCCL message
    ///
    /// \param hash Hash value of this message part
    /// \param desc Descriptor to validate. Use google::protobuf::Message::GetDescriptor() or MyProtobufType::descriptor() to get this object.
    /// \param part part of the Message
    void base_hash(std::size_t* hash, const google::protobuf::Descriptor* desc, MessagePart part);
    //@}

    /// \name Field functions (primitive types and embedded messages)
    //
    /// These are called typically by DefaultMessageCodec to start processing a new field. In this example "bar" and "baz" are fields:
    /// \code
    /// message Foo
    /// {
    ///    int32 bar = 1;
    ///    FooBar baz = 2;
    /// }
    /// \endcode
    //@{

    /// \brief Pre-encodes a non-repeated (i.e. optional or required) field by converting the FieldType representation (the Google Protobuf representation) into the WireType representation (the type used in the encoded DCCL message). This allows for type-converting codecs.
    ///
    /// \param wire_value Will be set to the converted field_value
    /// \param field_value Value to convert to the appropriate wire_value
    void field_pre_encode(dccl::any* wire_value, const dccl::any& field_value)
    {
        any_pre_encode(wire_value, field_value);
    }

    /// \brief Pre-encodes a repeated field.
    ///
    /// \param wire_values Should be set to the converted field_values
    /// \param field_values Values to convert to the appropriate wire_values
    void field_pre_encode_repeated(std::vector<dccl::any>* wire_values,
                                   const std::vector<dccl::any>& field_values)
    {
        any_pre_encode_repeated(wire_values, field_values);
    }

    // traverse const

    /// \brief Encode a non-repeated field.
    ///
    /// \param bits Pointer to bitset to store encoded bits. Bits are added to the most significant end of `bits`
    /// \param field_value Value to encode (FieldType)
    /// \param field Protobuf descriptor to the field to encode. Set to 0 for base message.
    void field_encode(Bitset* bits, const dccl::any& field_value,
                      const google::protobuf::FieldDescriptor* field);

    /// \brief Encode a repeated field.
    ///
    /// \param bits Pointer to bitset to store encoded bits. Bits are added to the most significant end of `bits`
    /// \param field_values Values to encode (FieldType)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_encode_repeated(Bitset* bits, const std::vector<dccl::any>& field_values,
                               const google::protobuf::FieldDescriptor* field);

    /// \brief Calculate the size of a field
    ///
    /// \param bit_size Location to <i>add</i> calculated bit size to. Be sure to zero `bit_size` if you want only the size of this field.
    /// \param field_value Value calculate size of (FieldType)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_size(unsigned* bit_size, const dccl::any& field_value,
                    const google::protobuf::FieldDescriptor* field);

    /// \brief Calculate the size of a repeated field
    ///
    /// \param bit_size Location to <i>add</i> calculated bit size to. Be sure to zero `bit_size` if you want only the size of this field.
    /// \param field_values Values to calculate size of (FieldType)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_size_repeated(unsigned* bit_size, const std::vector<dccl::any>& field_values,
                             const google::protobuf::FieldDescriptor* field);

    // traverse mutable
    /// \brief Decode a non-repeated field
    ///
    /// \param bits Bits to decode. Used bits are consumed (erased) from the least significant end
    /// \param field_value Location to store decoded value (FieldType)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_decode(Bitset* bits, dccl::any* field_value,
                      const google::protobuf::FieldDescriptor* field);

    /// \brief Decode a repeated field
    ///
    /// \param bits Bits to decode. Used bits are consumed (erased) from the least significant end
    /// \param field_values Location to store decoded values (FieldType)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_decode_repeated(Bitset* bits, std::vector<dccl::any>* field_values,
                               const google::protobuf::FieldDescriptor* field);

    /// \brief Post-decodes a non-repeated (i.e. optional or required) field by converting the WireType (the type used in the encoded DCCL message) representation into the FieldType representation (the Google Protobuf representation). This allows for type-converting codecs.
    ///
    /// \param wire_value Should be set to the desired value to translate
    /// \param field_value Will be set to the converted wire_value
    void field_post_decode(const dccl::any& wire_value, dccl::any* field_value)
    {
        any_post_decode(wire_value, field_value);
    }

    /// \brief Post-decodes a repeated field.
    ///
    /// \param wire_values Should be set to the desired values to translate
    /// \param field_values Will be set to the converted wire_values
    void field_post_decode_repeated(const std::vector<dccl::any>& wire_values,
                                    std::vector<dccl::any>* field_values)
    {
        any_post_decode_repeated(wire_values, field_values);
    }

    // traverse schema (Descriptor)

    /// \brief Calculate the upper bound on this field's size (in bits)
    ///
    /// \param bit_size Location to <i>add</i> calculated maximum bit size to. Be sure to zero `bit_size` if you want only the size of this field.
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_max_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);
    /// \brief Calculate the lower bound on this field's size (in bits)
    ///
    /// \param bit_size Location to <i>add</i> calculated minimum bit size to. Be sure to zero `bit_size` if you want only the size of this field.
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_min_size(unsigned* bit_size, const google::protobuf::FieldDescriptor* field);

    /// \brief Validate this field, checking that all required option extensions are set (e.g. (dccl.field).max and (dccl.field).min for arithmetic codecs)
    ///
    /// \param b Currently unused (will be set to false)
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    /// \throw Exception If field is invalid
    void field_validate(bool* b, const google::protobuf::FieldDescriptor* field);

    /// \brief Write human readable information about the field and its bounds to the provided stream.
    ///
    /// \param os Stream to write info to.
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_info(std::ostream* os, const google::protobuf::FieldDescriptor* field);

    /// \brief Provide a hash for this field definition
    ///
    /// \param hash Hash value of this field
    /// \param field Protobuf descriptor to the field. Set to 0 for base message.
    void field_hash(std::size_t* hash, const google::protobuf::FieldDescriptor* field);
    //@}

    /// \brief Get the DCCL field option extension value for the current field
    ///
    /// dccl::DCCLFieldOptions is defined in acomms_option_extensions.proto
    dccl::DCCLFieldOptions dccl_field_options() const
    {
        if (this_field())
            return this_field()->options().GetExtension(dccl::field);
        else
            throw(Exception(
                "Cannot call dccl_field on base message (has no *field* option extension"));
    }

    /// \brief Essentially an assertion to be used in the validate() virtual method
    ///
    /// \param b Boolean to assert (if true, execution continues, if false an exception is thrown)
    /// \param description Debugging description for this assertion (will be appended to the exception)
    /// \throw Exception Thrown if !b
    void require(bool b, const std::string& description)
    {
        if (!b)
        {
            if (this_field())
                throw(Exception("Field " + this_field()->name() +
                                    " failed validation: " + description,
                                this->this_descriptor()));
            else
                throw(Exception("Message " + this_descriptor()->name() +
                                    " failed validation: " + description,
                                this->this_descriptor()));
        }
    }

    DynamicConditions& dynamic_conditions(const google::protobuf::FieldDescriptor* field);

    FieldCodecManagerLocal& manager()
    {
        if (manager_)
            return *manager_;
        else
            throw(Exception("FieldCodecManagerLocal is not set"), this->this_descriptor());
    }

    const FieldCodecManagerLocal& manager() const
    {
        if (manager_)
            return *manager_;
        else
            throw(Exception("FieldCodecManagerLocal is not set"), this->this_descriptor());
    }

    virtual void set_manager(FieldCodecManagerLocal* manager) { manager_ = manager; }

  protected:
    /// \brief Whether to use the required or optional encoding
    bool use_required()
    {
        if (force_required_)
            return true;

        const google::protobuf::FieldDescriptor* field = this_field();
        DynamicConditions& dc = dynamic_conditions(field);
        // expensive, so don't do this unless we're going to use it
        if (dc.has_required_if())
            dc.regenerate(this_message(), root_message());

        if (!field)
            return true;
        else if (codec_version() > 3) // use required for repeated, required and oneof fields
            return field->is_required() || field->is_repeated() || is_part_of_oneof(field) ||
                   (dc.has_required_if() && dc.required());
        else if (codec_version() > 2) // use required for both repeated and required fields
            return field->is_required() || field->is_repeated() ||
                   (dc.has_required_if() && dc.required());
        else // use required only for required fields
            return field->is_required();
    }

    //
    // VIRTUAL
    //

    // contain dccl::any
    /// \brief Virtual method used to encode
    ///
    /// \param bits Bitset to store encoded bits. Bits is <i>just</i> the bits from the current operation (unlike base_encode() and field_encode() where bits are added to the most significant end).
    /// \param wire_value Value to encode (WireType)
    virtual void any_encode(Bitset* bits, const dccl::any& wire_value) = 0;

    /// \brief Virtual method used to decode
    ///
    /// \param bits Bitset containing bits to decode. This will initially contain min_size() bits. If you need more bits, call get_more_bits() with the number of bits required. This bits will be consumed from the bit pool and placed in `bits`.
    /// \param wire_value Place to store decoded value (as FieldType)
    virtual void any_decode(Bitset* bits, dccl::any* wire_value) = 0;

    /// \brief Virtual method used to pre-encode (convert from FieldType to WireType). The default implementation of this method is for when WireType == FieldType and simply copies the field_value to the wire_value.
    ///
    /// \param wire_value Converted value (WireType)
    /// \param field_value Value to convert (FieldType)
    virtual void any_pre_encode(dccl::any* wire_value, const dccl::any& field_value)
    {
        *wire_value = field_value;
    }

    /// \brief Virtual method used to post-decode (convert from WireType to FieldType). The default implementation of this method is for when WireType == FieldType and simply copies the wire_value to the field_value.
    ///
    /// \param wire_value Value to convert (WireType)
    /// \param field_value Converted value (FieldType)
    virtual void any_post_decode(const dccl::any& wire_value, dccl::any* field_value)
    {
        *field_value = wire_value;
    }

    /// \brief Virtual method for calculating the size of a field (in bits).
    ///
    /// \param wire_value Value to calculate size of
    /// \return Size of field (in bits)
    virtual unsigned any_size(const dccl::any& wire_value) = 0;

    // no dccl::any
    /// \brief Validate a field. Use require() inside your overloaded validate() to assert requirements or throw Exceptions directly as needed.
    virtual void validate() {}

    /// \brief Write field specific information (in addition to general information such as sizes that are automatically written by this class for all fields.
    ///
    /// \return string containing information to display.
    virtual std::string info();

    /// \brief Generate a field specific hash to be combined with the descriptor hash
    virtual std::size_t hash() { return 0; }

    /// \brief Calculate maximum size of the field in bits
    ///
    /// \return Maximum size of this field (in bits).
    virtual unsigned max_size() = 0;

    /// \brief Calculate minimum size of the field in bits
    ///
    /// \return Minimum size of this field (in bits).
    virtual unsigned min_size() = 0;

    virtual void any_encode_repeated(Bitset* bits, const std::vector<dccl::any>& wire_values);
    virtual void any_decode_repeated(Bitset* repeated_bits, std::vector<dccl::any>* field_values);

    virtual void any_pre_encode_repeated(std::vector<dccl::any>* wire_values,
                                         const std::vector<dccl::any>& field_values);

    virtual void any_post_decode_repeated(const std::vector<dccl::any>& wire_values,
                                          std::vector<dccl::any>* field_values);

    virtual unsigned any_size_repeated(const std::vector<dccl::any>& wire_values);
    virtual unsigned max_size_repeated();
    virtual unsigned min_size_repeated();
    void check_repeat_settings() const;

    friend class FieldCodecManagerLocal;

  private:
    // codec information
    void set_name(const std::string& name) { name_ = name; }
    void set_field_type(google::protobuf::FieldDescriptor::Type type) { field_type_ = type; }
    void set_wire_type(google::protobuf::FieldDescriptor::CppType type) { wire_type_ = type; }

    bool variable_size()
    {
        if (this_field() && this_field()->is_repeated())
            return max_size_repeated() != min_size_repeated();
        else
            return max_size() != min_size();
    }

    int repeated_vector_field_size(int min_repeat, int max_repeat)
    {
        return dccl::ceil_log2(max_repeat - min_repeat + 1);
    }

    void disp_size(const google::protobuf::FieldDescriptor* field, const Bitset& new_bits,
                   int depth, int vector_size = -1);

  private:
    // sets global statics relating the current message begin processed
    // and unsets them on destruction
    struct BaseRAII
    {
        BaseRAII(FieldCodecBase* field_codec, MessagePart part,
                 const google::protobuf::Descriptor* root_descriptor, bool strict = false);

        BaseRAII(FieldCodecBase* field_codec, MessagePart part,
                 const google::protobuf::Message* root_message, bool strict = false);
        ~BaseRAII();

      private:
        FieldCodecBase* field_codec_;
    };
    friend struct BaseRAII;

    std::string name_;
    google::protobuf::FieldDescriptor::Type field_type_;
    google::protobuf::FieldDescriptor::CppType wire_type_;

    bool force_required_{false};

    FieldCodecManagerLocal* manager_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const FieldCodecBase& field_codec);

inline Exception type_error(const std::string& action, const std::type_info& expected,
                            const std::type_info& got)
{
    std::string e = "error " + action + ", expected: ";
    e += expected.name();
    e += ", got ";
    e += got.name();
    return Exception(e);
}

} // namespace dccl

#endif
