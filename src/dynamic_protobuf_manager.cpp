// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.


#include <sstream>

#include "dynamic_protobuf_manager.h"
#include "logger.h"
#include "exception.h"

boost::shared_ptr<dccl::DynamicProtobufManager> dccl::DynamicProtobufManager::inst_;

const google::protobuf::FileDescriptor* dccl::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    simple_database().Add(proto);
    
    const google::protobuf::FileDescriptor* return_desc = user_descriptor_pool().FindFileByName(proto.name());
    return return_desc; 
}

void dccl::DynamicProtobufManager::enable_disk_source_database()
{
    if(disk_source_tree_)
        return;
    
    disk_source_tree_ = new google::protobuf::compiler::DiskSourceTree;
    source_database_ = new google::protobuf::compiler::SourceTreeDescriptorDatabase(disk_source_tree_);
    error_collector_ = new DLogMultiFileErrorCollector;
    
    source_database_->RecordErrorsTo(error_collector_);
    disk_source_tree_->MapPath("/", "/");
    disk_source_tree_->MapPath("", "");
    add_database(source_database_);
}


/** It is critical that the argument be the absolute, canonical path to the file.
 * This could be achieved, e.g., by using Boost filesystem as follows...
 * boost::filesystem::path abs_path = boost::filesystem::complete(rel_path);
 * abs_path.normalize();
 */
const google::protobuf::FileDescriptor*
dccl::DynamicProtobufManager::load_from_proto_file(const std::string& protofile_absolute_path)
{
    if(!get_instance()->source_database_)
        throw(dccl::Exception("Must called enable_compilation() before loading proto files directly"));

    return user_descriptor_pool().FindFileByName(protofile_absolute_path);
}


// DLogMultiFileErrorCollector
void dccl::DynamicProtobufManager::DLogMultiFileErrorCollector::AddError(const std::string & filename, int line, int column,
                                                                         const std::string & message)
{
    std::stringstream ss;
    ss << "File: " << filename
       << " has error (line: " << line << ", column: "
       << column << "):" << message;

    throw(dccl::Exception(ss.str()));

}


