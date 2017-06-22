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
#include "type_helper.h"

dccl::internal::TypeHelper::TypeMap dccl::internal::TypeHelper::type_map_;
dccl::internal::TypeHelper::CppTypeMap dccl::internal::TypeHelper::cpptype_map_;
dccl::internal::TypeHelper::CustomMessageMap dccl::internal::TypeHelper::custom_message_map_;

// used to construct, initialize, and delete a copy of this object
boost::shared_ptr<dccl::internal::TypeHelper> dccl::internal::TypeHelper::inst_(new dccl::internal::TypeHelper);

template<google::protobuf::FieldDescriptor::Type t>
void insertType(dccl::internal::TypeHelper::TypeMap* type_map)
{ type_map->insert(std::make_pair(t, boost::shared_ptr<dccl::internal::FromProtoTypeBase>(new dccl::internal::FromProtoType<t>))); }

template<google::protobuf::FieldDescriptor::CppType t>
void insert(dccl::internal::TypeHelper::CppTypeMap* cpptype_map)
{ cpptype_map->insert(std::make_pair(t, boost::shared_ptr<dccl::internal::FromProtoCppTypeBase>(new dccl::internal::FromProtoCppType<t>))); }


//
// TypeHelper
//
void dccl::internal::TypeHelper::initialize()
{
    using namespace google::protobuf;
    using boost::shared_ptr;
    using std::make_pair;
    
    type_map_.insert(make_pair(static_cast<FieldDescriptor::Type>(0),
                               shared_ptr<FromProtoTypeBase>(new FromProtoTypeBase)));
    insertType<FieldDescriptor::TYPE_DOUBLE>(&type_map_);
    insertType<FieldDescriptor::TYPE_FLOAT>(&type_map_);
    insertType<FieldDescriptor::TYPE_UINT64>(&type_map_);
    insertType<FieldDescriptor::TYPE_UINT32>(&type_map_);
    insertType<FieldDescriptor::TYPE_FIXED64>(&type_map_);
    insertType<FieldDescriptor::TYPE_FIXED32>(&type_map_);
    insertType<FieldDescriptor::TYPE_INT64>(&type_map_);
    insertType<FieldDescriptor::TYPE_INT32>(&type_map_);
    insertType<FieldDescriptor::TYPE_SFIXED32>(&type_map_);
    insertType<FieldDescriptor::TYPE_SFIXED64>(&type_map_);
    insertType<FieldDescriptor::TYPE_SINT32>(&type_map_);
    insertType<FieldDescriptor::TYPE_SINT64>(&type_map_);
    insertType<FieldDescriptor::TYPE_BOOL>(&type_map_);
    insertType<FieldDescriptor::TYPE_STRING>(&type_map_);
    insertType<FieldDescriptor::TYPE_BYTES>(&type_map_);
    insertType<FieldDescriptor::TYPE_MESSAGE>(&type_map_);
    insertType<FieldDescriptor::TYPE_GROUP>(&type_map_);
    insertType<FieldDescriptor::TYPE_ENUM>(&type_map_);


    cpptype_map_.insert(make_pair(static_cast<FieldDescriptor::CppType>(0),
                                  shared_ptr<FromProtoCppTypeBase>(new FromProtoCppTypeBase)));

    insert<FieldDescriptor::CPPTYPE_DOUBLE>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_FLOAT>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_UINT64>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_UINT32>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_INT64>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_INT32>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_BOOL>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_STRING>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_MESSAGE>(&cpptype_map_);
    insert<FieldDescriptor::CPPTYPE_ENUM>(&cpptype_map_);

}

boost::shared_ptr<dccl::internal::FromProtoCppTypeBase> dccl::internal::TypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name /*= ""*/)
{
    if(!type_name.empty())
    {
        CustomMessageMap::iterator it = custom_message_map_.find(type_name);
        if(it != custom_message_map_.end())
            return it->second;
    }
    
    CppTypeMap::iterator it = cpptype_map_.find(cpptype);
    if(it != cpptype_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoCppTypeBase>();
}


boost::shared_ptr<dccl::internal::FromProtoTypeBase> dccl::internal::TypeHelper::find(google::protobuf::FieldDescriptor::Type type)
{
    TypeMap::iterator it = type_map_.find(type);
    if(it != type_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoTypeBase>();
}

