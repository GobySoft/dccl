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
#include "field_codec.h"
#include "codec.h"
#include "exception.h"

using dccl::dlog;
using namespace dccl::logger;

//
// FieldCodecBase public
//
dccl::FieldCodecBase::FieldCodecBase() = default;

void dccl::FieldCodecBase::base_encode(Bitset* bits, const google::protobuf::Message& field_value,
                                       MessagePart part, bool strict)
{
    BaseRAII scoped_globals(this, part, &field_value, strict);

    // we pass this through the FromProtoCppTypeBase to do dynamic_cast (RTTI) for
    // custom message codecs so that these codecs can be written in the derived class (not google::protobuf::Message)
    field_encode(bits,
                 manager().type_helper().find(field_value.GetDescriptor())->get_value(field_value),
                 nullptr);
}

void dccl::FieldCodecBase::field_encode(Bitset* bits, const dccl::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (field)
        dlog.is(DEBUG2, ENCODE) && dlog << "Starting encode for field: " << field->DebugString()
                                        << std::flush;

    dccl::any wire_value;
    field_pre_encode(&wire_value, field_value);

    Bitset new_bits;
    any_encode(&new_bits, wire_value);
    disp_size(field, new_bits, msg_handler.field_size());
    bits->append(new_bits);

    if (field)
        dlog.is(DEBUG2, ENCODE) && dlog << "... produced these " << new_bits.size()
                                        << " bits: " << new_bits << std::endl;
}

void dccl::FieldCodecBase::field_encode_repeated(Bitset* bits,
                                                 const std::vector<dccl::any>& field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    std::vector<dccl::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);

    Bitset new_bits;
    any_encode_repeated(&new_bits, wire_values);
    disp_size(field, new_bits, msg_handler.field_size(), wire_values.size());
    bits->append(new_bits);
}

void dccl::FieldCodecBase::base_size(unsigned* bit_size, const google::protobuf::Message& msg,
                                     MessagePart part)
{
    BaseRAII scoped_globals(this, part, &msg);

    *bit_size = 0;

    field_size(bit_size, &msg, nullptr);
}

void dccl::FieldCodecBase::field_size(unsigned* bit_size, const dccl::any& field_value,
                                      const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    dccl::any wire_value;
    field_pre_encode(&wire_value, field_value);

    *bit_size += any_size(wire_value);
}

void dccl::FieldCodecBase::field_size_repeated(unsigned* bit_size,
                                               const std::vector<dccl::any>& field_values,
                                               const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    std::vector<dccl::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);

    *bit_size += any_size_repeated(wire_values);
}

void dccl::FieldCodecBase::base_decode(Bitset* bits, google::protobuf::Message* field_value,
                                       MessagePart part)
{
    BaseRAII scoped_globals(this, part, field_value);
    dccl::any value(field_value);
    field_decode(bits, &value, nullptr);
}

void dccl::FieldCodecBase::field_decode(Bitset* bits, dccl::any* field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (!field_value)
        throw(Exception("Decode called with NULL dccl::any"));
    else if (!bits)
        throw(Exception("Decode called with NULL Bitset"));

    if (field)
        dlog.is(DEBUG2, DECODE) && dlog << "Starting decode for field: " << field->DebugString()
                                        << std::flush;

    if (root_message())
        dlog.is(DEBUG3, DECODE) && dlog << "Message thus far is: " << root_message()->DebugString()
                                        << std::flush;

    Bitset these_bits(bits);

    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);

    if (field)
        dlog.is(DEBUG2, DECODE) && dlog << "... using these bits: " << these_bits << std::endl;

    dccl::any wire_value = *field_value;

    any_decode(&these_bits, &wire_value);

    field_post_decode(wire_value, field_value);
}

void dccl::FieldCodecBase::field_decode_repeated(Bitset* bits, std::vector<dccl::any>* field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (!field_values)
        throw(Exception("Decode called with NULL field_values"));
    else if (!bits)
        throw(Exception("Decode called with NULL Bitset"));

    if (field)
        dlog.is(DEBUG2, DECODE) &&
            dlog << "Starting repeated decode for field: " << field->DebugString() << std::endl;

    Bitset these_bits(bits);

    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);

    dlog.is(DEBUG2, DECODE) && dlog << "using these " << these_bits.size()
                                    << " bits: " << these_bits << std::endl;

    std::vector<dccl::any> wire_values = *field_values;
    any_decode_repeated(&these_bits, &wire_values);

    field_values->clear();
    field_post_decode_repeated(wire_values, field_values);
}

void dccl::FieldCodecBase::base_max_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc, MessagePart part)
{
    BaseRAII scoped_globals(this, part, desc);
    *bit_size = 0;

    internal::MessageStack msg_handler(root_message(), message_data());
    if (desc)
        msg_handler.push(desc);
    else
        throw(Exception("Max Size called with NULL Descriptor"));

    field_max_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(nullptr));
}

void dccl::FieldCodecBase::field_max_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (this_field())
        *bit_size += this_field()->is_repeated() ? max_size_repeated() : max_size();
    else
        *bit_size += max_size();
}

void dccl::FieldCodecBase::base_min_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc, MessagePart part)
{
    BaseRAII scoped_globals(this, part, desc);

    *bit_size = 0;

    internal::MessageStack msg_handler(root_message(), message_data());
    if (desc)
        msg_handler.push(desc);
    else
        throw(Exception("Min Size called with NULL Descriptor"));

    field_min_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(nullptr));
}

void dccl::FieldCodecBase::field_min_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)

{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (this_field())
        *bit_size += this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        *bit_size += min_size();
}

void dccl::FieldCodecBase::base_validate(const google::protobuf::Descriptor* desc, MessagePart part)
{
    BaseRAII scoped_globals(this, part, desc);

    internal::MessageStack msg_handler(root_message(), message_data());
    if (desc)
        msg_handler.push(desc);
    else
        throw(Exception("Validate called with NULL Descriptor"));

    bool b = false;
    field_validate(&b, static_cast<google::protobuf::FieldDescriptor*>(nullptr));
}

void dccl::FieldCodecBase::field_validate(bool* /*b*/,
                                          const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (field && dccl_field_options().in_head() && variable_size())
        throw(Exception("Variable size codec used in header - header fields must be encoded with "
                        "fixed size codec."));

    if (field && dccl_field_options().in_head() && is_part_of_oneof(field))
        throw(Exception(
            "Oneof field used in header - oneof fields cannot be encoded in the header."));

    validate();
}

void dccl::FieldCodecBase::base_info(std::ostream* os, const google::protobuf::Descriptor* desc,
                                     MessagePart part)
{
    BaseRAII scoped_globals(this, part, desc);

    internal::MessageStack msg_handler(root_message(), message_data());
    if (desc)
        msg_handler.push(desc);
    else
        throw(Exception("info called with NULL Descriptor"));

    field_info(os, static_cast<google::protobuf::FieldDescriptor*>(nullptr));
}

void dccl::FieldCodecBase::field_info(std::ostream* os,
                                      const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    std::stringstream ss;
    int depth = msg_handler.count();

    std::string name =
        ((this_field()) ? std::to_string(this_field()->number()) + ". " + this_field()->name()
                        : this_descriptor()->full_name());
    if (this_field() && this_field()->is_repeated())
        name += "[" +
                (dccl_field_options().has_min_repeat()
                     ? (std::to_string(dccl_field_options().min_repeat()) + "-")
                     : "") +
                std::to_string(dccl_field_options().max_repeat()) + "]";

    if (!this_field() || this_field()->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
        depth -= 1;

    const int spaces = 8;
    std::string indent =
        std::string(spaces * (depth) + spaces / 2 * (field && is_part_of_oneof(field)),
                    ' '); // Add 4 spaces of indentation for fields belonging to oneofs

    const int full_width = 40;

    bool is_zero_size = false;

    std::stringstream range;
    if (variable_size())
    {
        unsigned max_sz = 0, min_sz = 0;
        field_max_size(&max_sz, field);
        field_min_size(&min_sz, field);
        if (!max_sz)
            is_zero_size = true;
        range << min_sz << "-" << max_sz;
    }
    else
    {
        unsigned sz = 0;
        field_max_size(&sz, field);
        if (!sz)
            is_zero_size = true;
        range << sz;
    }

    int width = this_field() ? full_width - name.size() : full_width - name.size() + spaces;
    ss << indent << name << std::setfill('.') << std::setw(std::max(1, width)) << range.str()
       << " {"
       << (this_field() ? manager().find(this_field(), codec_version(), has_codec_group(), codec_group())->name()
                        : manager().find(manager().codec_data().root_descriptor_)->name())
       << "}";

    if (!is_zero_size)
        *os << ss.str() << "\n";

    std::string specific_info = info();
    if (!specific_info.empty())
        *os << specific_info;
}

void dccl::FieldCodecBase::base_hash(std::size_t* hash, const google::protobuf::Descriptor* desc,
                                     MessagePart part)
{
    BaseRAII scoped_globals(this, part, desc);

    internal::MessageStack msg_handler(root_message(), message_data());
    if (desc)
        msg_handler.push(desc);
    else
        throw(Exception("Hash called with NULL Descriptor"));

    field_hash(hash, static_cast<google::protobuf::FieldDescriptor*>(nullptr));
}

void dccl::FieldCodecBase::field_hash(std::size_t* hash_value,
                                      const google::protobuf::FieldDescriptor* field)
{
    internal::MessageStack msg_handler(root_message(), message_data(), field);

    if (field && dccl_field_options().in_head() && variable_size())
        throw(Exception("Variable size codec used in header - header fields must be encoded with "
                        "fixed size codec."));

    if (field && dccl_field_options().in_head() && is_part_of_oneof(field))
        throw(Exception(
            "Oneof field used in header - oneof fields cannot be encoded in the header."));

    if (this_field())
    {
        google::protobuf::FieldDescriptorProto field_to_hash;
        this_field()->CopyTo(&field_to_hash);

        // name doesn't affect encoding
        field_to_hash.clear_name();
        // we will handle dccl options separately, non-dccl options don't affect encoding
        field_to_hash.clear_options();
        field_to_hash.clear_type_name();
        field_to_hash.clear_default_value();

        dccl::DCCLFieldOptions dccl_opts = dccl_field_options();

        hash_combine(*hash_value, field_to_hash.DebugString());
        hash_combine(*hash_value, dccl_opts.DebugString());
    }
    else
    {
        // root level message
        dccl::DCCLMessageOptions dccl_opts = this_descriptor()->options().GetExtension(dccl::msg);
        dccl_opts.clear_max_bytes(); // max bytes doesn't affect encoding

        hash_combine(*hash_value, dccl_opts.DebugString());
    }

    hash_combine(*hash_value, hash());
}

std::string dccl::FieldCodecBase::codec_group(const google::protobuf::Descriptor* desc)
{
    if (desc->options().GetExtension(dccl::msg).has_codec_group())
        return desc->options().GetExtension(dccl::msg).codec_group();
    else
        return Codec::default_codec_name(desc->options().GetExtension(dccl::msg).codec_version());
}

//
// FieldCodecBase protected
//

std::string dccl::FieldCodecBase::info() { return std::string(); }

void dccl::FieldCodecBase::any_encode_repeated(dccl::Bitset* bits,
                                               const std::vector<dccl::any>& wire_values)
{
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]

    unsigned wire_vector_size = dccl_field_options().max_repeat();

    if (wire_values.size() > wire_vector_size && strict())
        throw(
            dccl::OutOfRangeException(std::string("Repeated size exceeds max_repeat for field: ") +
                                          FieldCodecBase::this_field()->DebugString(),
                                      this->this_field(), this->this_descriptor()));

    if (wire_values.size() < dccl_field_options().min_repeat() && strict())
        throw(dccl::OutOfRangeException(
            std::string("Repeated size is less than min_repeat for field: ") +
                FieldCodecBase::this_field()->DebugString(),
            this->this_field(), this->this_descriptor()));

    // for DCCL3 and beyond, add a prefix numeric field giving the vector size (rather than always going to max_repeat)
    if (codec_version() > 2)
    {
        wire_vector_size = std::min(static_cast<int>(dccl_field_options().max_repeat()),
                                    static_cast<int>(wire_values.size()));

        wire_vector_size = std::max(static_cast<int>(dccl_field_options().min_repeat()),
                                    static_cast<int>(wire_vector_size));

        Bitset size_bits(repeated_vector_field_size(dccl_field_options().min_repeat(),
                                                    dccl_field_options().max_repeat()),
                         wire_vector_size - dccl_field_options().min_repeat());
        bits->append(size_bits);

        dlog.is(DEBUG2, ENCODE) && dlog << "repeated size field ... produced these "
                                        << size_bits.size() << " bits: " << size_bits << std::endl;
    }

    internal::MessageStack msg_handler(root_message(), message_data(), this->this_field());
    for (unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        msg_handler.update_index(root_message(), this->this_field(), i);

        DynamicConditions& dc = this->dynamic_conditions(this->this_field());
        dc.set_repeated_index(i);
        if (dc.has_omit_if())
        {
            dc.regenerate(this_message(), root_message(), i);
            if (dc.omit())
                continue;
        }

        Bitset new_bits;
        if (i < wire_values.size())
            any_encode(&new_bits, wire_values[i]);
        else
            any_encode(&new_bits, dccl::any());
        bits->append(new_bits);
    }
}

void dccl::FieldCodecBase::any_decode_repeated(Bitset* repeated_bits,
                                               std::vector<dccl::any>* wire_values)
{
    unsigned wire_vector_size = dccl_field_options().max_repeat();
    if (codec_version() > 2)
    {
        Bitset size_bits(repeated_bits);
        size_bits.get_more_bits(repeated_vector_field_size(dccl_field_options().min_repeat(),
                                                           dccl_field_options().max_repeat()));

        wire_vector_size = size_bits.to_ulong() + dccl_field_options().min_repeat();
    }

    wire_values->resize(wire_vector_size);

    internal::MessageStack msg_handler(root_message(), message_data(), this->this_field());
    for (unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        msg_handler.update_index(root_message(), this->this_field(), i);

        DynamicConditions& dc = this->dynamic_conditions(this->this_field());
        dc.set_repeated_index(i);
        if (dc.has_omit_if())
        {
            dc.regenerate(this_message(), root_message(), i);
            if (dc.omit())
                continue;
        }

        Bitset these_bits(repeated_bits);
        these_bits.get_more_bits(min_size());
        any_decode(&these_bits, &(*wire_values)[i]);
    }
}

unsigned dccl::FieldCodecBase::any_size_repeated(const std::vector<dccl::any>& wire_values)
{
    unsigned out = 0;
    unsigned wire_vector_size = dccl_field_options().max_repeat();

    if (codec_version() > 2)
    {
        wire_vector_size = std::min(static_cast<int>(dccl_field_options().max_repeat()),
                                    static_cast<int>(wire_values.size()));
        out += repeated_vector_field_size(dccl_field_options().min_repeat(),
                                          dccl_field_options().max_repeat());
    }

    internal::MessageStack msg_handler(root_message(), message_data(), this->this_field());
    for (unsigned i = 0, n = wire_vector_size; i < n; ++i)
    {
        msg_handler.update_index(root_message(), this->this_field(), i);
        DynamicConditions& dc = this->dynamic_conditions(this->this_field());
        dc.set_repeated_index(i);
        if (dc.has_omit_if())
        {
            dc.regenerate(this_message(), root_message(), i);
            if (dc.omit())
                continue;
        }

        if (i < wire_values.size())
            out += any_size(wire_values[i]);
        else
            out += any_size(dccl::any());
    }
    return out;
}

void dccl::FieldCodecBase::check_repeat_settings() const
{
    if (!dccl_field_options().has_max_repeat())
        throw(Exception("Missing (dccl.field).max_repeat option on `repeated` field: " +
                            this_field()->DebugString(),
                        this->this_descriptor()));
    else if (dccl_field_options().max_repeat() < 1)
        throw(Exception("(dccl.field).max_repeat must not be less than 1: " +
                            this_field()->DebugString(),
                        this->this_descriptor()));
    else if (dccl_field_options().max_repeat() < dccl_field_options().min_repeat())
        throw(Exception("(dccl.field).max_repeat must not be less than (dccl.field).min_repeat: " +
                            this_field()->DebugString(),
                        this->this_descriptor()));
}

unsigned dccl::FieldCodecBase::max_size_repeated()
{
    check_repeat_settings();

    if (codec_version() > 2)
        return repeated_vector_field_size(dccl_field_options().min_repeat(),
                                          dccl_field_options().max_repeat()) +
               max_size() * dccl_field_options().max_repeat();
    else
        return max_size() * dccl_field_options().max_repeat();
}

unsigned dccl::FieldCodecBase::min_size_repeated()
{
    check_repeat_settings();

    if (codec_version() > 2)
        return repeated_vector_field_size(dccl_field_options().min_repeat(),
                                          dccl_field_options().max_repeat()) +
               min_size() * dccl_field_options().min_repeat();

    else
        return min_size() * dccl_field_options().max_repeat();
}

void dccl::FieldCodecBase::any_pre_encode_repeated(std::vector<dccl::any>* wire_values,
                                                   const std::vector<dccl::any>& field_values)
{
    for (const auto& field_value : field_values)
    {
        dccl::any wire_value;
        any_pre_encode(&wire_value, field_value);
        wire_values->push_back(wire_value);
    }
}
void dccl::FieldCodecBase::any_post_decode_repeated(const std::vector<dccl::any>& wire_values,
                                                    std::vector<dccl::any>* field_values)
{
    for (const auto& wire_value : wire_values)
    {
        dccl::any field_value;
        any_post_decode(wire_value, &field_value);
        field_values->push_back(field_value);
    }
}

//
// FieldCodecBase private
//

void dccl::FieldCodecBase::disp_size(const google::protobuf::FieldDescriptor* field,
                                     const Bitset& new_bits, int depth, int vector_size /* = -1 */)
{
    if (!root_descriptor())
        return;

    if (dlog.check(DEBUG2))
    {
        std::string name = ((field) ? field->name() : root_descriptor()->full_name());
        if (vector_size >= 0)
            name += "[" + std::to_string(vector_size) + "]";

        dlog.is(DEBUG2, SIZE) && dlog << std::string(depth, '|') << name << std::setfill('.')
                                      << std::setw(40 - name.size() - depth) << new_bits.size()
                                      << std::endl;

        if (!field)
            dlog.is(DEBUG2, SIZE) && dlog << std::endl;
    }
}

std::ostream& dccl::operator<<(std::ostream& os, const dccl::FieldCodecBase& field_codec)
{
    using google::protobuf::FieldDescriptor;
    return os << "[FieldCodec '" << field_codec.name() << "']: field type: "
              << field_codec.manager().type_helper().find(field_codec.field_type())->as_str()
              << " ("
              << field_codec.manager()
                     .type_helper()
                     .find(FieldDescriptor::TypeToCppType(field_codec.field_type()))
                     ->as_str()
              << ") | wire type: "
              << field_codec.manager().type_helper().find(field_codec.wire_type())->as_str();
}

const google::protobuf::FieldDescriptor* dccl::FieldCodecBase::this_field() const
{
    return message_data().top_field();
}

const google::protobuf::Descriptor* dccl::FieldCodecBase::this_descriptor() const
{
    return message_data().top_descriptor();
}

const google::protobuf::Message* dccl::FieldCodecBase::this_message()
{
    return message_data().top_message();
}

const google::protobuf::Message* dccl::FieldCodecBase::root_message()
{
    return manager().codec_data().root_message_;
}

const google::protobuf::Descriptor* dccl::FieldCodecBase::root_descriptor() const
{
    return manager().codec_data().root_descriptor_;
}

dccl::internal::MessageStackData& dccl::FieldCodecBase::message_data()
{
    return manager().codec_data().message_data_;
}

const dccl::internal::MessageStackData& dccl::FieldCodecBase::message_data() const
{
    return manager().codec_data().message_data_;
}
bool dccl::FieldCodecBase::has_codec_group()
{
    const google::protobuf::Descriptor* root_desc = root_descriptor();
    if (root_desc)
    {
        return root_desc->options().GetExtension(dccl::msg).has_codec_group() ||
               root_desc->options().GetExtension(dccl::msg).has_codec_version();
    }
    else
        return false;
}

dccl::MessagePart dccl::FieldCodecBase::part() { return manager().codec_data().part_; }

bool dccl::FieldCodecBase::strict() { return manager().codec_data().strict_; }

int dccl::FieldCodecBase::codec_version()
{
    return root_descriptor()->options().GetExtension(dccl::msg).codec_version();
}

std::string dccl::FieldCodecBase::codec_group() { return codec_group(root_descriptor()); }

dccl::DynamicConditions&
dccl::FieldCodecBase::dynamic_conditions(const google::protobuf::FieldDescriptor* field)
{
    manager().codec_data().dynamic_conditions_.set_field(field);
    return manager().codec_data().dynamic_conditions_;
}

dccl::FieldCodecBase::BaseRAII::BaseRAII(FieldCodecBase* field_codec, MessagePart part,
                                         const google::protobuf::Descriptor* root_descriptor,
                                         bool strict)
    : field_codec_(field_codec)

{
    field_codec_->manager().codec_data().part_ = part;
    field_codec_->manager().codec_data().strict_ = strict;
    field_codec_->manager().codec_data().root_message_ = nullptr;
    field_codec_->manager().codec_data().root_descriptor_ = root_descriptor;
}
dccl::FieldCodecBase::BaseRAII::BaseRAII(FieldCodecBase* field_codec, MessagePart part,
                                         const google::protobuf::Message* root_message, bool strict)
    : field_codec_(field_codec)

{
    field_codec_->manager().codec_data().part_ = part;
    field_codec_->manager().codec_data().strict_ = strict;
    field_codec_->manager().codec_data().root_message_ = root_message;
    field_codec_->manager().codec_data().root_descriptor_ = root_message->GetDescriptor();
}
dccl::FieldCodecBase::BaseRAII::~BaseRAII()
{
    field_codec_->manager().codec_data().part_ = dccl::UNKNOWN;
    field_codec_->manager().codec_data().strict_ = false;
    field_codec_->manager().codec_data().root_message_ = nullptr;
    field_codec_->manager().codec_data().root_descriptor_ = nullptr;
}
