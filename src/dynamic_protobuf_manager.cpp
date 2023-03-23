// Copyright 2012-2019:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
#include <sstream>

#include "dynamic_protobuf_manager.h"
#include "exception.h"
#include "logger.h"

std::shared_ptr<dccl::DynamicProtobufManager> dccl::DynamicProtobufManager::inst_;

//
// STATIC
//
const google::protobuf::Descriptor*
dccl::DynamicProtobufManager::find_descriptor(const std::string& protobuf_type_name,
                                              bool user_pool_first)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    const google::protobuf::Descriptor* desc = nullptr;
    if (user_pool_first)
    {
        // try the user pool
        desc = get_instance()->user_descriptor_pool_->FindMessageTypeByName(protobuf_type_name);
        if (desc)
            return desc;
    }

    // try the generated pool
    desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
        protobuf_type_name);
    if (desc)
        return desc;

    if (!user_pool_first)
    {
        // try the user pool
        desc = get_instance()->user_descriptor_pool_->FindMessageTypeByName(protobuf_type_name);
    }

    return desc;
}

std::shared_ptr<google::protobuf::Message>
dccl::DynamicProtobufManager::new_protobuf_message(const google::protobuf::Descriptor* desc)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    return new_protobuf_message<std::shared_ptr<google::protobuf::Message>>(desc);
}

std::shared_ptr<google::protobuf::Message>
dccl::DynamicProtobufManager::new_protobuf_message(const std::string& protobuf_type_name)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    return new_protobuf_message<std::shared_ptr<google::protobuf::Message>>(protobuf_type_name);
}

void dccl::DynamicProtobufManager::add_database(
    std::shared_ptr<google::protobuf::DescriptorDatabase> database)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    get_instance()->databases_.push_back(database);
    get_instance()->update_databases();
}

void dccl::DynamicProtobufManager::add_include_path(const std::string& path)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX

    if (!get_instance()->disk_source_tree_)
        throw(std::runtime_error(
            "Must called enable_compilation() before loading proto files directly"));

    get_instance()->disk_source_tree_->MapPath("", path);
}

void* dccl::DynamicProtobufManager::load_from_shared_lib(const std::string& shared_lib_path)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    void* handle = dlopen(shared_lib_path.c_str(), RTLD_LAZY);
    if (handle)
        get_instance()->dl_handles_.push_back(handle);
    return handle;
}

/** It is critical that the argument be the absolute, canonical path to the file.
 * This could be achieved, e.g., by using Boost filesystem as follows...
 * boost::filesystem::path abs_path = boost::filesystem::complete(rel_path);
 * abs_path.normalize();
 */
const google::protobuf::FileDescriptor*
dccl::DynamicProtobufManager::load_from_proto_file(const std::string& protofile_absolute_path)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX

    if (!get_instance()->source_database_)
        throw(dccl::Exception(
            "Must called enable_compilation() before loading proto files directly"));

    return get_instance()->user_descriptor_pool_->FindFileByName(protofile_absolute_path);
}

const google::protobuf::FileDescriptor*
dccl::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
    get_instance()->simple_database_->Add(proto);

    const google::protobuf::FileDescriptor* return_desc =
        get_instance()->user_descriptor_pool_->FindFileByName(proto.name());
    return return_desc;
}

//
// NON STATIC
//

void dccl::DynamicProtobufManager::enable_disk_source_database()
{
    if (disk_source_tree_)
        return;

    disk_source_tree_.reset(new google::protobuf::compiler::DiskSourceTree);
    source_database_.reset(
        new google::protobuf::compiler::SourceTreeDescriptorDatabase(disk_source_tree_.get()));
    error_collector_.reset(new DLogMultiFileErrorCollector);

    source_database_->RecordErrorsTo(error_collector_.get());
    disk_source_tree_->MapPath("/", "/");
    disk_source_tree_->MapPath("", "");
    add_database(source_database_);
}

// DLogMultiFileErrorCollector
void dccl::DynamicProtobufManager::DLogMultiFileErrorCollector::AddError(
    const std::string& filename, int line, int column, const std::string& message)
{
    std::stringstream ss;
    ss << "File: " << filename << " has error (line: " << line << ", column: " << column
       << "):" << message;

    throw(dccl::Exception(ss.str()));
}
