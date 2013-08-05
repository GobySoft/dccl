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



#include "type_helper.h"

dccl::TypeHelper::TypeMap dccl::TypeHelper::type_map_;
dccl::TypeHelper::CppTypeMap dccl::TypeHelper::cpptype_map_;
dccl::TypeHelper::CustomMessageMap dccl::TypeHelper::custom_message_map_;

// used to construct, initialize, and delete a copy of this object
boost::shared_ptr<dccl::TypeHelper> dccl::TypeHelper::inst_(new dccl::TypeHelper);

template<google::protobuf::FieldDescriptor::Type t>
void insert(dccl::TypeHelper::TypeMap* type_map)
{ type_map->insert(std::make_pair(t, boost::shared_ptr<dccl::FromProtoTypeBase>(new dccl::FromProtoType<t>))); }

template<google::protobuf::FieldDescriptor::CppType t>
void insert(dccl::TypeHelper::CppTypeMap* cpptype_map)
{ cpptype_map->insert(std::make_pair(t, boost::shared_ptr<dccl::FromProtoCppTypeBase>(new dccl::FromProtoCppType<t>))); }


//
// TypeHelper
//
void dccl::TypeHelper::initialize()
{
    using namespace google::protobuf;
    using boost::shared_ptr;
    using std::make_pair;
    
    type_map_.insert(make_pair(static_cast<FieldDescriptor::Type>(0),
                               shared_ptr<FromProtoTypeBase>(new FromProtoTypeBase)));
    insert<FieldDescriptor::TYPE_DOUBLE>(&type_map_);
    insert<FieldDescriptor::TYPE_FLOAT>(&type_map_);
    insert<FieldDescriptor::TYPE_UINT64>(&type_map_);
    insert<FieldDescriptor::TYPE_UINT32>(&type_map_);
    insert<FieldDescriptor::TYPE_FIXED64>(&type_map_);
    insert<FieldDescriptor::TYPE_FIXED32>(&type_map_);
    insert<FieldDescriptor::TYPE_INT64>(&type_map_);
    insert<FieldDescriptor::TYPE_INT32>(&type_map_);
    insert<FieldDescriptor::TYPE_SFIXED32>(&type_map_);
    insert<FieldDescriptor::TYPE_SFIXED64>(&type_map_);
    insert<FieldDescriptor::TYPE_SINT32>(&type_map_);
    insert<FieldDescriptor::TYPE_SINT64>(&type_map_);
    insert<FieldDescriptor::TYPE_BOOL>(&type_map_);
    insert<FieldDescriptor::TYPE_STRING>(&type_map_);
    insert<FieldDescriptor::TYPE_BYTES>(&type_map_);
    insert<FieldDescriptor::TYPE_MESSAGE>(&type_map_);
    insert<FieldDescriptor::TYPE_GROUP>(&type_map_);
    insert<FieldDescriptor::TYPE_ENUM>(&type_map_);


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

boost::shared_ptr<dccl::FromProtoCppTypeBase> dccl::TypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype, const std::string& type_name /*= ""*/)
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


boost::shared_ptr<dccl::FromProtoTypeBase> dccl::TypeHelper::find(google::protobuf::FieldDescriptor::Type type)
{
    TypeMap::iterator it = type_map_.find(type);
    if(it != type_map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoTypeBase>();
}

