// Copyright 2012-2022:
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
#ifndef DCCLDYNAMICPROTOBUFMANAGER20110419H
#define DCCLDYNAMICPROTOBUFMANAGER20110419H

#include <dlfcn.h>

#include <iostream>
#include <set>
#include <stdexcept>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>

#include <boost/shared_ptr.hpp>
#include <boost/version.hpp>

#include "thread_safety.h"

namespace dccl
{
/// Helper class for creating google::protobuf::Message objects that are not statically compiled into the application.
class DynamicProtobufManager
{
  public:
    /// \brief Finds the Google Protobuf Descriptor (essentially a meta-class for a given Message) from a given Message name.
    ///
    /// \param protobuf_type_name The fully qualified name of the Google Protobuf Message including package name. E.g. in the .proto file:
    /// \code
    /// package dccl.protobuf
    /// message A { }
    /// \endcode
    /// would result in protobuf_type_name == "dccl.protobuf.A"
    /// \param user_pool_first Search the user pool first, then the generated (compiled-in) pool (useful in case the generated pool is missing extensions that are in the user pool)
    /// \return A pointer to the google::protobuf::Descriptor (or nullptr if not found)
    static const google::protobuf::Descriptor*
    find_descriptor(const std::string& protobuf_type_name, bool user_pool_first = false);

    /// \brief Create a new (empty) Google Protobuf message of a given type by name.
    ///
    /// \param protobuf_type_name The fully qualified name of the Google Protobuf Message including package name. E.g. in the .proto file:
    /// \code
    /// package dccl.protobuf
    /// message A { }
    /// \endcode
    /// would result in protobuf_type_name == "dccl.protobuf.A"
    /// \tparam GoogleProtobufMessagePointer A pointer or anything that acts like a pointer (has operator*()) to a google::protobuf::Message
    /// \return A pointer to the newly created object. Deleting the memory is up to the caller of this function, so smart pointers (e.g. boost::shared_ptr<google::protobuf::Message>) are recommended.
    template <typename GoogleProtobufMessagePointer>
    static GoogleProtobufMessagePointer new_protobuf_message(const std::string& protobuf_type_name,
                                                             bool user_pool_first = false)
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX

        const google::protobuf::Descriptor* desc =
            find_descriptor(protobuf_type_name, user_pool_first);
        if (desc)
            return new_protobuf_message<GoogleProtobufMessagePointer>(desc);
        else
            throw(std::runtime_error("Unknown type " + protobuf_type_name +
                                     ", be sure it is loaded at compile-time, via dlopen, or with "
                                     "a call to add_protobuf_file()"));
    }

    /// \brief Create a new (empty) Google Protobuf message of a given type by Descriptor
    ///
    /// \param desc The Google Protobuf Descriptor of the message to create.
    /// \tparam GoogleProtobufMessagePointer A pointer or anything that acts like a pointer (has operator*()) to a google::protobuf::Message
    /// \return A pointer to the newly created object. Deleting the memory is up to the caller of this function, so smart pointers (e.g. boost::shared_ptr<google::protobuf::Message>) are recommended.
    template <typename GoogleProtobufMessagePointer>
    static GoogleProtobufMessagePointer
    new_protobuf_message(const google::protobuf::Descriptor* desc)
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
        return GoogleProtobufMessagePointer(
            get_instance()->msg_factory_->GetPrototype(desc)->New());
    }

    /// \brief Create a new (empty) Google Protobuf message of a given type by Descriptor
    ///
    /// \param desc The Google Protobuf Descriptor of the message to create.
    /// \return A boost::shared_ptr to the newly created object.
    static boost::shared_ptr<google::protobuf::Message>
    new_protobuf_message(const google::protobuf::Descriptor* desc);

    /// \brief Create a new (empty) Google Protobuf message of a given type by name.
    ///
    /// \param protobuf_type_name The full name (including package) of the Google Protobuf message to create (e.g. "package.MyMessage").
    /// \return A boost::shared_ptr to the newly created object.
    static boost::shared_ptr<google::protobuf::Message>
    new_protobuf_message(const std::string& protobuf_type_name);

    /// \brief Add a Google Protobuf DescriptorDatabase to the set of databases searched for Message Descriptors.
    static void add_database(boost::shared_ptr<google::protobuf::DescriptorDatabase> database);

    /// \brief Enable on the fly compilation of .proto files on the local disk. Must be called before load_from_proto_file() is called.
    static void enable_compilation()
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
        get_instance()->enable_disk_source_database();
    }

    /// \brief Load a message from a .proto file on the disk. enable_compilation() must be called first.
    ///
    /// \param protofile_absolute_path It is critical that the argument be the absolute, canonical path to the file. (No relative paths and no "." or "..")
    /// This could be achieved, e.g., by using Boost filesystem as follows...
    /// \code
    /// boost::filesystem::path abs_path = boost::filesystem::complete(rel_path);
    /// abs_path.normalize();
    /// \endcode
    /// \throw Exception If enable_compilation() has not been called before using this function
    /// \throw Exception If any error exists in locating or parsing this .proto file.
    static const google::protobuf::FileDescriptor*
    load_from_proto_file(const std::string& protofile_absolute_path);

    /// \brief Add a path for searching for import messages when loading .proto files using load_from_proto_file()
    ///
    /// \throw Exception If enable_compilation() has not been called before using this function.
    static void add_include_path(const std::string& path);

    /// \brief Load compiled .proto files from a UNIX shared library (i.e. *.so or *.dylib)
    ///
    /// \param shared_lib_path Path to shared library. May be relative if known by ld.so
    static void* load_from_shared_lib(const std::string& shared_lib_path);

    static void protobuf_shutdown()
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
        get_instance()->shutdown();
    }

    /// \brief Add a protobuf file defined in a google::protobuf::FileDescriptorProto
    static const google::protobuf::FileDescriptor*
    add_protobuf_file(const google::protobuf::FileDescriptorProto& proto);

    static void reset()
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
        inst_.reset(new DynamicProtobufManager);
    }

#if !(DCCL_THREAD_SUPPORT)
    // no way to make these thread safe without the downstream user locking the mutex

    static google::protobuf::DynamicMessageFactory& msg_factory()
    {
        return *get_instance()->msg_factory_;
    }
    static google::protobuf::DescriptorPool& user_descriptor_pool()
    {
        return *get_instance()->user_descriptor_pool_;
    }
    static google::protobuf::SimpleDescriptorDatabase& simple_database()
    {
        return *get_instance()->simple_database_;
    }
#endif

  private:
    // so we can use shared_ptr to hold the singleton
#if BOOST_VERSION >= 107000
    template <typename T> friend void boost::checked_delete(T*) BOOST_NOEXCEPT;
#else
    template <typename T> friend void boost::checked_delete(T*);
#endif

    static boost::shared_ptr<DynamicProtobufManager> inst_;

    static DynamicProtobufManager* get_instance()
    {
        LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX

        if (!inst_)
            inst_.reset(new DynamicProtobufManager);
        return inst_.get();
    }

    DynamicProtobufManager()
        : generated_database_(new google::protobuf::DescriptorPoolDatabase(
              *google::protobuf::DescriptorPool::generated_pool())),
          simple_database_(new google::protobuf::SimpleDescriptorDatabase),
          msg_factory_(new google::protobuf::DynamicMessageFactory)
    {
        databases_.push_back(simple_database_);
        databases_.push_back(generated_database_);

        msg_factory_->SetDelegateToGeneratedFactory(true);

        update_databases();
    }

    ~DynamicProtobufManager() {}

    void shutdown()
    {
        for (std::vector<void*>::iterator it = dl_handles_.begin(), n = dl_handles_.end(); it != n;
             ++it)
            dlclose(*it);
        google::protobuf::ShutdownProtobufLibrary();
        inst_.reset();
    }

    void update_databases()
    {
        std::vector<google::protobuf::DescriptorDatabase*> databases;

        for (std::vector<boost::shared_ptr<google::protobuf::DescriptorDatabase>>::const_iterator
                 it = databases_.begin(),
                 end = databases_.end();
             it != end; ++it)
            databases.push_back(it->get());

        merged_database_.reset(new google::protobuf::MergedDescriptorDatabase(databases));
        user_descriptor_pool_.reset(new google::protobuf::DescriptorPool(merged_database_.get()));
    }

    void enable_disk_source_database();

    DynamicProtobufManager(const DynamicProtobufManager&) = delete;
    DynamicProtobufManager& operator=(const DynamicProtobufManager&) = delete;

  private:
    std::vector<boost::shared_ptr<google::protobuf::DescriptorDatabase>> databases_;

    // always used
    boost::shared_ptr<google::protobuf::DescriptorPoolDatabase> generated_database_;
    boost::shared_ptr<google::protobuf::SimpleDescriptorDatabase> simple_database_;
    boost::shared_ptr<google::protobuf::MergedDescriptorDatabase> merged_database_;
    boost::shared_ptr<google::protobuf::DescriptorPool> user_descriptor_pool_;
    boost::shared_ptr<google::protobuf::DynamicMessageFactory> msg_factory_;

    // sometimes used
    boost::shared_ptr<google::protobuf::compiler::DiskSourceTree> disk_source_tree_;
    boost::shared_ptr<google::protobuf::compiler::SourceTreeDescriptorDatabase> source_database_;

    class DLogMultiFileErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
    {
        void AddError(const std::string& filename, int line, int column,
                      const std::string& message);
    };

    boost::shared_ptr<DLogMultiFileErrorCollector> error_collector_;

    std::vector<void*> dl_handles_;
};

} // namespace dccl

#endif
