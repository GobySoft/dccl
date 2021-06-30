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
#ifndef TypeHelper20110405H
#define TypeHelper20110405H

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/version.hpp>

#include "protobuf_cpp_type_helpers.h"

namespace dccl
{
    class FieldCodecManager;
    
    namespace internal
    {
        
        /// \brief Provides FromProtoTypeBase and FromProtoCppTypeBase type identification helper classes for various representations of the underlying field.
        class TypeHelper
        {
          public:
            static boost::shared_ptr<FromProtoTypeBase> find(google::protobuf::FieldDescriptor::Type type);
            static boost::shared_ptr<FromProtoCppTypeBase> find(const google::protobuf::FieldDescriptor* field)
            {
                if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    return find(field->message_type());
                else
                    return find(field->cpp_type());
            }
                
            static boost::shared_ptr<FromProtoCppTypeBase> find(const google::protobuf::Descriptor* desc)
            {
                return find(google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE,
                            desc->full_name());
            }
                
            static boost::shared_ptr<FromProtoCppTypeBase> find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name = "");
            
          private:
            friend class ::dccl::FieldCodecManager;
            template<typename ProtobufMessage>
                static void add()
            {
                custom_message_map_.insert(std::make_pair(ProtobufMessage::descriptor()->full_name(),
                                                          boost::shared_ptr<FromProtoCppTypeBase>(new FromProtoCustomMessage<ProtobufMessage>)));
            }
            template<typename ProtobufMessage>
                static void remove()
            {
                custom_message_map_.erase(ProtobufMessage::descriptor()->full_name());
            }
            static void reset()
            {
                inst_.reset();
            }


            TypeHelper() { initialize(); }            
            ~TypeHelper()
            {
                type_map_.clear();
                cpptype_map_.clear();
                custom_message_map_.clear();
            }
            TypeHelper(const TypeHelper&);
            TypeHelper& operator= (const TypeHelper&);
            void initialize();    
            
          public:
            // so we can use shared_ptr to hold the singleton
#if BOOST_VERSION >= 107000
        template<typename T>
            friend void boost::checked_delete(T*) BOOST_NOEXCEPT;
#else
        template<typename T>
            friend void boost::checked_delete(T*);
#endif
            static boost::shared_ptr<TypeHelper> inst_;
            
            typedef std::map<google::protobuf::FieldDescriptor::Type,
                boost::shared_ptr<FromProtoTypeBase> > TypeMap;
            static TypeMap type_map_;

            typedef std::map<google::protobuf::FieldDescriptor::CppType,
                boost::shared_ptr<FromProtoCppTypeBase> > CppTypeMap;
            static CppTypeMap cpptype_map_;

            typedef std::map<std::string,
                boost::shared_ptr<FromProtoCppTypeBase> > CustomMessageMap;
            static CustomMessageMap custom_message_map_;
        };
    }
}

#endif
