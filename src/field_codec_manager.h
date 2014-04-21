// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.



#ifndef FieldCodecManager20110405H
#define FieldCodecManager20110405H

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/logical.hpp>

#include "type_helper.h"
#include "field_codec.h"
#include "dccl/logger.h"

namespace dccl
{
    // See Bug #1089061, and Boost MPL Documentation section 3.4 (compiler workarounds)
    template <int> struct dummy_fcm { dummy_fcm(int) {} };

    /// \todo (tes) Make sanity check for newly added FieldCodecs
    class FieldCodecManager
    {
      public:
        // field type == wire type
        /* template<typename FieldType, template <typename FieldType> class Codec> */
        /*     static void add(const std::string& name); */
            
        /// \brief Add a new field codec (used for codecs operating on statically generated Protobuf messages, that is, children of google::protobuf::Message but not google::protobuf::Message itself).
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
        template<class Codec>
            typename boost::enable_if<
            boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
            >,
            void>::type 
            static add(const std::string& name, dummy_fcm<0> dummy_fcm = 0);
                
        /// \brief Add a new field codec (used for codecs operating on all types except statically generated Protobuf messages).
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
        template<class Codec>
            typename boost::disable_if<
            boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message,typename Codec::wire_type> >
            >,
            void>::type
            static add(const std::string& name, dummy_fcm<1> dummy_fcm = 0);
                
        /// \brief Add a new field codec only valid for a specific google::protobuf::FieldDescriptor::Type. This is useful if a given codec is designed to work with only a specific Protobuf type that shares an underlying C++ type (e.g. Protobuf types `bytes` and `string`)
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \tparam type The google::protobuf::FieldDescriptor::Type enumeration that this codec works on.
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        template<class Codec, google::protobuf::FieldDescriptor::Type type>
            static void add(const std::string& name);

        /// \brief Remove a new field codec (used for codecs operating on statically generated Protobuf messages, that is, children of google::protobuf::Message but not google::protobuf::Message itself).
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper remove() overload.
        template<class Codec>
            typename boost::enable_if<
            boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
            >,
            void>::type 
            static remove(const std::string& name, dummy_fcm<0> dummy_fcm = 0);
                
        /// \brief Remove a new field codec (used for codecs operating on all types except statically generated Protobuf messages).
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper remove() overload.
        template<class Codec>
            typename boost::disable_if<
            boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
            boost::mpl::not_<boost::is_same<google::protobuf::Message,typename Codec::wire_type> >
            >,
            void>::type
            static remove(const std::string& name, dummy_fcm<1> dummy_fcm = 0);
                
        /// \brief Remove a new field codec only valid for a specific google::protobuf::FieldDescriptor::Type. This is useful if a given codec is designed to work with only a specific Protobuf type that shares an underlying C++ type (e.g. Protobuf types `bytes` and `string`)
        ///
        /// \tparam Codec A child of FieldCodecBase
        /// \tparam type The google::protobuf::FieldDescriptor::Type enumeration that this codec works on.
        /// \param name Name to use for this codec. Corresponds to (dccl.field).codec="name" in .proto file.
        template<class Codec, google::protobuf::FieldDescriptor::Type type>
            static void remove(const std::string& name);

        
        /// \brief Find the codec for a given field. For embedded messages, prefers (dccl.field).codec (inside field) over (dccl.msg).codec (inside embedded message).
        static boost::shared_ptr<FieldCodecBase> find(
            const google::protobuf::FieldDescriptor* field,
            bool has_codec_group,
            const std::string& codec_group)
        {
            std::string name = __find_codec(field, has_codec_group, codec_group);            
            
            if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                return find(field->message_type(), name);
            else
                return __find(field->type(), name);
        }                

        /// \brief Find the codec for a given base (or embedded) message.
        ///
        /// \param desc Message descriptor to find codec for
        /// \param name Codec name (used for embedded messages to prefer the codec listed as a field option). Omit for finding the codec of a base message (one that is not embedded).
        static boost::shared_ptr<FieldCodecBase> find(
            const google::protobuf::Descriptor* desc,
            std::string name = "")
        {
            // this was called on the root message
            if(name.empty())
            {
                // explicitly declared codec takes precedence over group
                if(desc->options().GetExtension(dccl::msg).has_codec())
                    name = desc->options().GetExtension(dccl::msg).codec();
                else
                    name = FieldCodecBase::codec_group(desc);
            }
            
            return __find(google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                          name, desc->full_name());
        }

        static boost::shared_ptr<FieldCodecBase> find(
            google::protobuf::FieldDescriptor::Type type,
            std::string name)
        {
            return __find(type, name);
        }

        static void clear()
        {
            TypeHelper::reset();
            codecs_.clear();
        }
        
        
      private:
        FieldCodecManager() { }
        ~FieldCodecManager() { }
        FieldCodecManager(const FieldCodecManager&);
        FieldCodecManager& operator= (const FieldCodecManager&);

                
            
        static boost::shared_ptr<FieldCodecBase> __find(
            google::protobuf::FieldDescriptor::Type type,
            const std::string& codec_name,
            const std::string& type_name = "");
            
        static std::string __mangle_name(const std::string& codec_name,
                                         const std::string& type_name) 
        { return type_name.empty() ? codec_name : codec_name + "[" + type_name + "]"; }
                

        template<typename WireType, typename FieldType, class Codec> 
            static void add_all_types(const std::string& name); 

        template<class Codec>
            static void add_single_type(const std::string& name,
                                        google::protobuf::FieldDescriptor::Type field_type,
                                        google::protobuf::FieldDescriptor::CppType wire_type);
        
        template<typename WireType, typename FieldType, class Codec> 
            static void remove_all_types(const std::string& name); 

        template<class Codec>
            static void remove_single_type(const std::string& name,
                                        google::protobuf::FieldDescriptor::Type field_type,
                                        google::protobuf::FieldDescriptor::CppType wire_type);

        
        static std::string __find_codec(const google::protobuf::FieldDescriptor* field,
                                        bool has_codec_group, const std::string& codec_group)
        {
            dccl::DCCLFieldOptions dccl_field_options = field->options().GetExtension(dccl::field);
                
            // prefer the codec listed as a field extension
            if(dccl_field_options.has_codec())
                return dccl_field_options.codec();                
            // then, the codec embedded in the message option extension
            else if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && field->message_type()->options().GetExtension(dccl::msg).has_codec())
                return field->message_type()->options().GetExtension(dccl::msg).codec();
            // then the overarching codec group
            else if(has_codec_group)
                return codec_group;
            // finally the default
            else
                return dccl_field_options.codec();
        }

      private:
        typedef std::map<std::string, boost::shared_ptr<FieldCodecBase> > InsideMap;
        static std::map<google::protobuf::FieldDescriptor::Type, InsideMap> codecs_;
    };
}

template<class Codec>
typename boost::enable_if<
boost::mpl::and_<
boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
>,
void>::type 
dccl::FieldCodecManager::add(const std::string& name, dummy_fcm<0> dummy_fcm)
{
    TypeHelper::add<typename Codec::wire_type>();
    add_single_type<Codec>(__mangle_name(name, Codec::wire_type::descriptor()->full_name()),
                           google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                           google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template<class Codec>
typename boost::disable_if<
boost::mpl::and_<
boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
    >,
    void>::type
    dccl::FieldCodecManager::add(const std::string& name, dummy_fcm<1> dummy_fcm)
{
    add_all_types<typename Codec::wire_type, typename Codec::field_type, Codec>(name);
}

template<class Codec, google::protobuf::FieldDescriptor::Type type> 
    void dccl::FieldCodecManager::add(const std::string& name) 
{ 
    add_single_type<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
}


template<typename WireType, typename FieldType, class Codec>
    void dccl::FieldCodecManager::add_all_types(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_field_type = ToProtoCppType<FieldType>::as_enum();
    const FieldDescriptor::CppType cpp_wire_type = ToProtoCppType<WireType>::as_enum();

    for(int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type field_type = static_cast<FieldDescriptor::Type>(i);
        if(FieldDescriptor::TypeToCppType(field_type) == cpp_field_type)
        {            
            add_single_type<Codec>(name, field_type, cpp_wire_type);
        }
    }
}

template<class Codec>
void dccl::FieldCodecManager::add_single_type(const std::string& name,
                                              google::protobuf::FieldDescriptor::Type field_type,
                                              google::protobuf::FieldDescriptor::CppType wire_type)
{
    using google::protobuf::FieldDescriptor;
    if(!codecs_[field_type].count(name))
    {
        boost::shared_ptr<FieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        codecs_[field_type][name] = new_field_codec;
        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog << "Adding codec " << *new_field_codec << std::endl;
    }            
    else
    {
        boost::shared_ptr<FieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog << "Trying to add: " << *new_field_codec
                                                            << ", but already have duplicate codec (For `name`/`field type` pair) "
                                                            << *(codecs_[field_type].find(name)->second)
                                                            << std::endl;
    }
}



template<class Codec>
typename boost::enable_if<
boost::mpl::and_<
boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
>,
void>::type 
dccl::FieldCodecManager::remove(const std::string& name, dummy_fcm<0> dummy_fcm)
{
    TypeHelper::remove<typename Codec::wire_type>();
    remove_single_type<Codec>(__mangle_name(name, Codec::wire_type::descriptor()->full_name()),
                              google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                              google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template<class Codec>
typename boost::disable_if<
boost::mpl::and_<
boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
    >,
    void>::type
    dccl::FieldCodecManager::remove(const std::string& name, dummy_fcm<1> dummy_fcm)
{
    remove_all_types<typename Codec::wire_type, typename Codec::field_type, Codec>(name);
}

template<class Codec, google::protobuf::FieldDescriptor::Type type> 
    void dccl::FieldCodecManager::remove(const std::string& name) 
{ 
    remove_single_type<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
}


template<typename WireType, typename FieldType, class Codec>
    void dccl::FieldCodecManager::remove_all_types(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_field_type = ToProtoCppType<FieldType>::as_enum();
    const FieldDescriptor::CppType cpp_wire_type = ToProtoCppType<WireType>::as_enum();

    for(int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type field_type = static_cast<FieldDescriptor::Type>(i);
        if(FieldDescriptor::TypeToCppType(field_type) == cpp_field_type)
        {            
            remove_single_type<Codec>(name, field_type, cpp_wire_type);
        }
    }
}

template<class Codec>
void dccl::FieldCodecManager::remove_single_type(const std::string& name,
                                              google::protobuf::FieldDescriptor::Type field_type,
                                              google::protobuf::FieldDescriptor::CppType wire_type)
{
    using google::protobuf::FieldDescriptor;
    if(codecs_[field_type].count(name))
    {       
        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog << "Removing codec " << *codecs_[field_type][name]  << std::endl;
        codecs_[field_type].erase(name);
    }            
    else
    {
        boost::shared_ptr<FieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        dccl::dlog.is(dccl::logger::DEBUG1) && dccl::dlog << "Trying to remove: " << *new_field_codec
                                                            << ", but no such codec exists"
                                                            << std::endl;
    }
}




#endif
