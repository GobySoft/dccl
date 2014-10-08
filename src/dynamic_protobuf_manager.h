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



#ifndef DCCLDYNAMICPROTOBUFMANAGER20110419H
#define DCCLDYNAMICPROTOBUFMANAGER20110419H

#include <dlfcn.h>

#include <set>
#include <stdexcept>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/compiler/importer.h>

#include <boost/shared_ptr.hpp>

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
        static const google::protobuf::Descriptor* find_descriptor(const std::string& protobuf_type_name)
        {
            // try the generated pool
            const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(protobuf_type_name);
            if(desc) return desc;
                
            // try the user pool
            desc = user_descriptor_pool().FindMessageTypeByName(protobuf_type_name);
            return desc;
        }

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
        template<typename GoogleProtobufMessagePointer>
            static GoogleProtobufMessagePointer new_protobuf_message(
                const std::string& protobuf_type_name)
        {
            const google::protobuf::Descriptor* desc = find_descriptor(protobuf_type_name);
            if(desc)
                return new_protobuf_message<GoogleProtobufMessagePointer>(desc);
            else
                throw(std::runtime_error("Unknown type " + protobuf_type_name + ", be sure it is loaded at compile-time, via dlopen, or with a call to add_protobuf_file()"));
        }
            
        /// \brief Create a new (empty) Google Protobuf message of a given type by Descriptor
        ///
        /// \param desc The Google Protobuf Descriptor of the message to create.
        /// \tparam GoogleProtobufMessagePointer A pointer or anything that acts like a pointer (has operator*()) to a google::protobuf::Message
        /// \return A pointer to the newly created object. Deleting the memory is up to the caller of this function, so smart pointers (e.g. boost::shared_ptr<google::protobuf::Message>) are recommended.
        template<typename GoogleProtobufMessagePointer>
            static GoogleProtobufMessagePointer new_protobuf_message( 
                const google::protobuf::Descriptor* desc)
        { return GoogleProtobufMessagePointer(msg_factory().GetPrototype(desc)->New()); }
            
        /// \brief Create a new (empty) Google Protobuf message of a given type by Descriptor
        ///
        /// \param desc The Google Protobuf Descriptor of the message to create.
        /// \return A boost::shared_ptr to the newly created object.
        static boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
            const google::protobuf::Descriptor* desc)
        { return new_protobuf_message<boost::shared_ptr<google::protobuf::Message> >(desc); }
            
        /// \brief Create a new (empty) Google Protobuf message of a given type by name.
        ///
        /// \param desc The Google Protobuf Descriptor of the message to create.
        /// \return A boost::shared_ptr to the newly created object.
        static boost::shared_ptr<google::protobuf::Message> new_protobuf_message(
            const std::string& protobuf_type_name)
        { return new_protobuf_message<boost::shared_ptr<google::protobuf::Message> >(protobuf_type_name); }
            
            
        /// \brief Add a Google Protobuf DescriptorDatabase to the set of databases searched for Message Descriptors.
        static void add_database(google::protobuf::DescriptorDatabase* database)
        {
            get_instance()->databases_.push_back(database);
            get_instance()->update_databases();
        }

        /// \brief Enable on the fly compilation of .proto files on the local disk. Must be called before load_from_proto_file() is called.
        static void enable_compilation()
        {
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
        static void add_include_path(const std::string& path)
        {
            if(!get_instance()->disk_source_tree_)
                throw(std::runtime_error("Must called enable_compilation() before loading proto files directly"));

            get_instance()->disk_source_tree_->MapPath("", path);
        }

        /// \brief Load compiled .proto files from a UNIX shared library (i.e. *.so or *.dylib)
        ///
        /// \param shared_lib_path Path to shared library. May be relative if known by ld.so
        static void* load_from_shared_lib(const std::string& shared_lib_path)
        {
            void* handle = dlopen(shared_lib_path.c_str(), RTLD_LAZY);
            if(handle)
                get_instance()->dl_handles_.push_back(handle);
            return handle;
        }
        
        static void protobuf_shutdown()
        {
            get_instance()->shutdown();
        }
            
        
        /// \brief Add a protobuf file defined in a google::protobuf::FileDescriptorProto
        static const google::protobuf::FileDescriptor* add_protobuf_file(
            const google::protobuf::FileDescriptorProto& proto);

        static google::protobuf::DynamicMessageFactory& msg_factory()
        { return *get_instance()->msg_factory_; }
        static google::protobuf::DescriptorPool& user_descriptor_pool()
        { return *get_instance()->user_descriptor_pool_; }
        static google::protobuf::SimpleDescriptorDatabase& simple_database()
        { return *get_instance()->simple_database_; }
            
      private:
        // so we can use shared_ptr to hold the singleton
        template<typename T>
            friend void boost::checked_delete(T*);
        static boost::shared_ptr<DynamicProtobufManager> inst_;

        static DynamicProtobufManager* get_instance()
        {
            if(!inst_)
                inst_.reset(new DynamicProtobufManager);
            return inst_.get();
        }
            
      DynamicProtobufManager()
          : generated_database_(new google::protobuf::DescriptorPoolDatabase(*google::protobuf::DescriptorPool::generated_pool())),
            simple_database_(new google::protobuf::SimpleDescriptorDatabase),
            msg_factory_(new google::protobuf::DynamicMessageFactory),
            disk_source_tree_(0),
            source_database_(0),
            error_collector_(0)                
            {
                databases_.push_back(simple_database_); 
                databases_.push_back(generated_database_);

                merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
                user_descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
            }
            
        ~DynamicProtobufManager()
        {
        }

        void shutdown()
        {

            delete msg_factory_;
            delete user_descriptor_pool_;
            delete merged_database_;
            delete simple_database_;
            delete generated_database_;

            if(disk_source_tree_)
                delete disk_source_tree_;
            if(source_database_)
                delete source_database_;
            if(error_collector_)
                delete error_collector_;
                
            google::protobuf::ShutdownProtobufLibrary();

            for(std::vector<void *>::iterator it = dl_handles_.begin(),
                    n = dl_handles_.end(); it != n; ++it)
                dlclose(*it);
        }
            
            
        void update_databases()
        {
            delete user_descriptor_pool_;
            delete merged_database_;
                
            merged_database_ = new google::protobuf::MergedDescriptorDatabase(databases_);
            user_descriptor_pool_ = new google::protobuf::DescriptorPool(merged_database_);
        }

        void enable_disk_source_database();
            
        DynamicProtobufManager(const DynamicProtobufManager&);
        DynamicProtobufManager& operator= (const DynamicProtobufManager&);
            
      private:
        std::vector<google::protobuf::DescriptorDatabase *> databases_;

        // always used
        google::protobuf::DescriptorPoolDatabase* generated_database_;
        google::protobuf::SimpleDescriptorDatabase* simple_database_;
        google::protobuf::MergedDescriptorDatabase* merged_database_;
        google::protobuf::DescriptorPool* user_descriptor_pool_;
        google::protobuf::DynamicMessageFactory* msg_factory_;

        // sometimes used
        google::protobuf::compiler::DiskSourceTree* disk_source_tree_;
        google::protobuf::compiler::SourceTreeDescriptorDatabase* source_database_;

        class DLogMultiFileErrorCollector
            : public google::protobuf::compiler::MultiFileErrorCollector
        {
            void AddError(const std::string & filename, int line, int column,
                          const std::string & message);
        };
            
        DLogMultiFileErrorCollector* error_collector_;

        std::vector<void *> dl_handles_;
    };
        
}

#endif
