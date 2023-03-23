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
#ifndef FieldCodecManager20110405H
#define FieldCodecManager20110405H

#include <boost/mpl/and.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

#include "dccl/logger.h"
#include "dccl/thread_safety.h"
#include "field_codec.h"
#include "internal/field_codec_data.h"
#include "internal/type_helper.h"

namespace dccl
{
namespace compiler
{
/// See Bug #1089061, and Boost MPL Documentation section 3.4 (compiler workarounds)
template <int> struct dummy_fcm
{
    dummy_fcm(int) {}
};
} // namespace compiler

/// \brief A class for managing the various field codecs. Here you can add and remove field codecs. The DCCL Codec and DefaultMessageCodec use the find() methods to locate the appropriate field codec.
class FieldCodecManagerLocal
{
  public:
    FieldCodecManagerLocal();
    ~FieldCodecManagerLocal();

    // field type == wire type
    /* template<typename FieldType, template <typename FieldType> class Codec> */
    /*      void add(const std::string& name); */

    /// \brief Add a new field codec (used for codecs operating on statically generated Protobuf messages, that is, children of google::protobuf::Message but not google::protobuf::Message itself).
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
    template <class Codec>
    typename boost::enable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    add(const std::string& name, compiler::dummy_fcm<0> dummy_fcm = 0);

    /// \brief Add a new field codec (used for codecs operating on all types except statically generated Protobuf messages).
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
    template <class Codec>
    typename boost::disable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    add(const std::string& name, compiler::dummy_fcm<1> dummy_fcm = 0);

    /// \brief Add a new field codec only valid for a specific google::protobuf::FieldDescriptor::Type. This is useful if a given codec is designed to work with only a specific Protobuf type that shares an underlying C++ type (e.g. Protobuf types `bytes` and `string`)
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \tparam type The google::protobuf::FieldDescriptor::Type enumeration that this codec works on.
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    template <class Codec, google::protobuf::FieldDescriptor::Type type>
    void add(const std::string& name);

    /// \brief Remove a new field codec (used for codecs operating on statically generated Protobuf messages, that is, children of google::protobuf::Message but not google::protobuf::Message itself).
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper remove() overload.
    template <class Codec>
    typename boost::enable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    remove(const std::string& name, compiler::dummy_fcm<0> dummy_fcm = 0);

    /// \brief Remove a new field codec (used for codecs operating on all types except statically generated Protobuf messages).
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper remove() overload.
    template <class Codec>
    typename boost::disable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    remove(const std::string& name, compiler::dummy_fcm<1> dummy_fcm = 0);

    /// \brief Remove a new field codec only valid for a specific google::protobuf::FieldDescriptor::Type. This is useful if a given codec is designed to work with only a specific Protobuf type that shares an underlying C++ type (e.g. Protobuf types `bytes` and `string`)
    ///
    /// \tparam Codec A child of FieldCodecBase
    /// \tparam type The google::protobuf::FieldDescriptor::Type enumeration that this codec works on.
    /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
    template <class Codec, google::protobuf::FieldDescriptor::Type type>
    void remove(const std::string& name);

    /// \brief Find the codec for a given field. For embedded messages, prefers (dccl.field).codec (inside field) over (dccl.msg).codec (inside embedded message).
    boost::shared_ptr<FieldCodecBase> find(const google::protobuf::FieldDescriptor* field,
                                           bool has_codec_group,
                                           const std::string& codec_group) const
    {
        std::string name = __find_codec(field, has_codec_group, codec_group);

        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            return find(field->message_type(), name);
        else
            return __find(field->type(), name);
    }

    /// \brief Find the codec for a given base (or embedded) message.
    ///
    /// \param desc Message descriptor to find codec for
    /// \param name Codec name (used for embedded messages to prefer the codec listed as a field option). Omit for finding the codec of a base message (one that is not embedded).
    boost::shared_ptr<FieldCodecBase> find(const google::protobuf::Descriptor* desc,
                                           std::string name = "") const
    {
        // this was called on the root message
        if (name.empty())
        {
            // explicitly declared codec takes precedence over group
            if (desc->options().GetExtension(dccl::msg).has_codec())
                name = desc->options().GetExtension(dccl::msg).codec();
            else
                name = FieldCodecBase::codec_group(desc);
        }

        return __find(google::protobuf::FieldDescriptor::TYPE_MESSAGE, name, desc->full_name());
    }

    boost::shared_ptr<FieldCodecBase> find(google::protobuf::FieldDescriptor::Type type,
                                           std::string name) const
    {
        return __find(type, name);
    }

    void clear()
    {
        type_helper_.reset();
        codecs_.clear();
    }

    internal::TypeHelper& type_helper() { return type_helper_; }
    const internal::TypeHelper& type_helper() const { return type_helper_; }

    internal::CodecData& codec_data() { return codec_data_; }
    const internal::CodecData& codec_data() const { return codec_data_; }

  private:
    boost::shared_ptr<FieldCodecBase> __find(google::protobuf::FieldDescriptor::Type type,
                                             const std::string& codec_name,
                                             const std::string& type_name = "") const;

    std::string __mangle_name(const std::string& codec_name, const std::string& type_name) const
    {
        return type_name.empty() ? codec_name : codec_name + "[" + type_name + "]";
    }

    template <typename WireType, typename FieldType, class Codec>
    void add_all_types(const std::string& name);

    template <class Codec>
    void add_single_type(const std::string& name,
                         google::protobuf::FieldDescriptor::Type field_type,
                         google::protobuf::FieldDescriptor::CppType wire_type);

    void add_single_type(boost::shared_ptr<FieldCodecBase> new_field_codec, const std::string& name,
                         google::protobuf::FieldDescriptor::Type field_type,
                         google::protobuf::FieldDescriptor::CppType wire_type);

    template <typename WireType, typename FieldType, class Codec>
    void remove_all_types(const std::string& name);

    template <class Codec>
    void remove_single_type(const std::string& name,
                            google::protobuf::FieldDescriptor::Type field_type,
                            google::protobuf::FieldDescriptor::CppType wire_type);

    std::string __find_codec(const google::protobuf::FieldDescriptor* field, bool has_codec_group,
                             const std::string& codec_group) const
    {
        dccl::DCCLFieldOptions dccl_field_options = field->options().GetExtension(dccl::field);

        // prefer the codec listed as a field extension
        if (dccl_field_options.has_codec())
            return dccl_field_options.codec();
        // then, the codec embedded in the message option extension
        else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE &&
                 field->message_type()->options().GetExtension(dccl::msg).has_codec())
            return field->message_type()->options().GetExtension(dccl::msg).codec();
        // then the overarching codec group
        else if (has_codec_group)
            return codec_group;
        // finally the default
        else
            return dccl_field_options.codec();
    }

  private:
    typedef std::map<std::string, boost::shared_ptr<FieldCodecBase>> InsideMap;
    std::map<google::protobuf::FieldDescriptor::Type, InsideMap> codecs_;

    internal::TypeHelper type_helper_;
    internal::CodecData codec_data_;
};

class FieldCodecManager
{
  public:
    template <class Codec>
    static typename boost::enable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    add(const std::string& name, compiler::dummy_fcm<0> dummy_fcm = 0)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global `FieldCodecManager::add<...>(...)` is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().add<...>(...)` instead, or for the ID "
                      "Codec use `dccl::Codec codec(\"id_codec_name\", MyIDCodec())`.");
    }

    template <class Codec>
    static typename boost::disable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    add(const std::string& name, compiler::dummy_fcm<1> dummy_fcm = 0)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global `FieldCodecManager::add<...>(...)` is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().add<...>(...)` instead.`");
    }

    template <class Codec, google::protobuf::FieldDescriptor::Type type>
    static void add(const std::string& name)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global `FieldCodecManager::add<...>(...)` is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().add<...>(...)` instead, or for the ID "
                      "Codec use `dccl::Codec codec(\"id_codec_name\", MyIDCodec())`.");
    }

    template <class Codec>
    static typename boost::enable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    remove(const std::string& name, compiler::dummy_fcm<0> dummy_fcm = 0)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global FieldCodecManager::remove<...>(...) is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().remove<...>(...) instead");
    }

    template <class Codec>
    static typename boost::disable_if<
        boost::mpl::and_<
            boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
        void>::type
    remove(const std::string& name, compiler::dummy_fcm<1> dummy_fcm = 0)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global FieldCodecManager::remove<...>(...) is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().remove<...>(...) instead");
    }

    template <class Codec, google::protobuf::FieldDescriptor::Type type>
    static void remove(const std::string& name)
    {
        static_assert(sizeof(Codec) == 0,
                      "The global FieldCodecManager::remove<...>(...) is no longer available. Use "
                      "`dccl::Codec codec; codec.manager().remove<...>(...) instead");
    }

}; // namespace dccl
} // namespace dccl

template <class Codec>
typename boost::enable_if<
    boost::mpl::and_<
        boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
        boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
    void>::type
dccl::FieldCodecManagerLocal::add(const std::string& name, compiler::dummy_fcm<0> dummy_fcm)
{
    type_helper_.add<typename Codec::wire_type>();
    add_single_type<Codec>(__mangle_name(name, Codec::wire_type::descriptor()->full_name()),
                           google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                           google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template <class Codec>
typename boost::disable_if<
    boost::mpl::and_<
        boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
        boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
    void>::type
dccl::FieldCodecManagerLocal::add(const std::string& name, compiler::dummy_fcm<1> dummy_fcm)
{
    add_all_types<typename Codec::wire_type, typename Codec::field_type, Codec>(name);
}

template <class Codec, google::protobuf::FieldDescriptor::Type type>
void dccl::FieldCodecManagerLocal::add(const std::string& name)
{
    add_single_type<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
}

template <typename WireType, typename FieldType, class Codec>
void dccl::FieldCodecManagerLocal::add_all_types(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_field_type = internal::ToProtoCppType<FieldType>::as_enum();
    const FieldDescriptor::CppType cpp_wire_type = internal::ToProtoCppType<WireType>::as_enum();

    for (int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type field_type = static_cast<FieldDescriptor::Type>(i);
        if (FieldDescriptor::TypeToCppType(field_type) == cpp_field_type)
        {
            add_single_type<Codec>(name, field_type, cpp_wire_type);
        }
    }
}

template <class Codec>
void dccl::FieldCodecManagerLocal::add_single_type(
    const std::string& name, google::protobuf::FieldDescriptor::Type field_type,
    google::protobuf::FieldDescriptor::CppType wire_type)
{
    boost::shared_ptr<FieldCodecBase> new_field_codec(new Codec());
    new_field_codec->set_name(name);
    new_field_codec->set_field_type(field_type);
    new_field_codec->set_wire_type(wire_type);
    new_field_codec->set_manager(this);
    using google::protobuf::FieldDescriptor;
    if (!codecs_[field_type].count(name))
    {
        codecs_[field_type][name] = new_field_codec;
        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog << "Adding codec " << *new_field_codec
                                                          << std::endl;
    }
    else
    {
        dccl::dlog.is(dccl::logger::DEBUG1) &&
            dccl::dlog << "Trying to add: " << *new_field_codec
                       << ", but already have duplicate codec (For `name`/`field type` pair) "
                       << *(codecs_[field_type].find(name)->second) << std::endl;
    }
}

template <class Codec>
typename boost::enable_if<
    boost::mpl::and_<
        boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
        boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
    void>::type
dccl::FieldCodecManagerLocal::remove(const std::string& name, compiler::dummy_fcm<0> dummy_fcm)
{
    type_helper_.remove<typename Codec::wire_type>();
    remove_single_type<Codec>(__mangle_name(name, Codec::wire_type::descriptor()->full_name()),
                              google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                              google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template <class Codec>
typename boost::disable_if<
    boost::mpl::and_<
        boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
        boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type>>>,
    void>::type
dccl::FieldCodecManagerLocal::remove(const std::string& name, compiler::dummy_fcm<1> dummy_fcm)
{
    remove_all_types<typename Codec::wire_type, typename Codec::field_type, Codec>(name);
}

template <class Codec, google::protobuf::FieldDescriptor::Type type>
void dccl::FieldCodecManagerLocal::remove(const std::string& name)
{
    remove_single_type<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
}

template <typename WireType, typename FieldType, class Codec>
void dccl::FieldCodecManagerLocal::remove_all_types(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_field_type = internal::ToProtoCppType<FieldType>::as_enum();
    const FieldDescriptor::CppType cpp_wire_type = internal::ToProtoCppType<WireType>::as_enum();

    for (int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type field_type = static_cast<FieldDescriptor::Type>(i);
        if (FieldDescriptor::TypeToCppType(field_type) == cpp_field_type)
        {
            remove_single_type<Codec>(name, field_type, cpp_wire_type);
        }
    }
}

template <class Codec>
void dccl::FieldCodecManagerLocal::remove_single_type(
    const std::string& name, google::protobuf::FieldDescriptor::Type field_type,
    google::protobuf::FieldDescriptor::CppType wire_type)
{
    using google::protobuf::FieldDescriptor;
    if (codecs_[field_type].count(name))
    {
        dccl::dlog.is(dccl::logger::DEBUG1) &&
            dccl::dlog << "Removing codec " << *codecs_[field_type][name] << std::endl;
        codecs_[field_type].erase(name);
    }
    else
    {
        boost::shared_ptr<FieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        new_field_codec->set_manager(this);

        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog
                                                   << "Trying to remove: " << *new_field_codec
                                                   << ", but no such codec exists" << std::endl;
    }
}

#endif
