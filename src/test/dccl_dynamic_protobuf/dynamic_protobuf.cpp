// Copyright 2019-2023:
//   GobySoft, LLC (2013-)
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
#include "../../dynamic_protobuf_manager.h"
#include <cassert>
#include <dlfcn.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <iostream>
#include "dccl/logger.h"

#include "test_a.pb.h"

int main(int argc, char* argv[])
{
    bool verbose = false;
    const char* lib_path = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
        {
            verbose = true;
        }
        else if (!lib_path)
        {
            lib_path = argv[i];
        }
        else
        {
            lib_path = nullptr;
            break;
        }
    }

    if (!lib_path)
    {
        std::cerr << "Usage: " << argv[0] << " [-v] /path/to/libtest_dyn_protobuf" << std::endl;
        exit(1);
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    void* dl_handle = dlopen(lib_path, RTLD_LAZY);

    if (!dl_handle)
    {
        std::cerr << "Failed to open libtest_dyn_protobuf" SHARED_LIBRARY_SUFFIX
                  << ", error: " << dlerror() << std::endl;
        exit(1);
    }

    std::shared_ptr<google::protobuf::SimpleDescriptorDatabase> simple_database(
        new google::protobuf::SimpleDescriptorDatabase);
    dccl::DynamicProtobufManager::add_database(simple_database);

    {
        // testing compiled in
        std::shared_ptr<google::protobuf::Message> adyn_msg =
            dccl::DynamicProtobufManager::new_protobuf_message("A");

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << adyn_msg->GetDescriptor()->DebugString() << std::endl;

        // testing dlopen'd
        std::shared_ptr<google::protobuf::Message> bdyn_msg =
            dccl::DynamicProtobufManager::new_protobuf_message("B");

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << bdyn_msg->GetDescriptor()->DebugString() << std::endl;

        // test non-existent
        try
        {
            std::shared_ptr<google::protobuf::Message> cdyn_msg =
                dccl::DynamicProtobufManager::new_protobuf_message("C");
            // should throw / avoid static analyzer error here
            dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << cdyn_msg->GetDescriptor()->DebugString() << std::endl;
            assert(false);
        }
        catch (std::exception& e)
        {
            // expected
        }

        // test dynamically loaded
        google::protobuf::FileDescriptorProto d_proto;
        std::string d_proto_str = "name: \"goby/test/util/dynamic_protobuf/test_d.proto\" "
                                  "message_type {   name: \"D\"   field {     name: \"d1\"     "
                                  "number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";

        google::protobuf::TextFormat::ParseFromString(d_proto_str, &d_proto);
        dccl::DynamicProtobufManager::add_protobuf_file(d_proto);

        std::shared_ptr<google::protobuf::Message> ddyn_msg =
            dccl::DynamicProtobufManager::new_protobuf_message("D");

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << ddyn_msg->GetDescriptor()->DebugString() << std::endl;

        // test dynamically via separate database
        google::protobuf::FileDescriptorProto e_proto;
        std::string e_proto_str = "name: \"goby/test/util/dynamic_protobuf/test_e.proto\" "
                                  "message_type {   name: \"E\"   field {     name: \"e1\"     "
                                  "number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";

        google::protobuf::TextFormat::ParseFromString(e_proto_str, &e_proto);

        simple_database->Add(e_proto);

        std::shared_ptr<google::protobuf::Message> edyn_msg =
            dccl::DynamicProtobufManager::new_protobuf_message("E");
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << edyn_msg->GetDescriptor()->DebugString() << std::endl;

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "all tests passed" << std::endl;
    }

    dccl::DynamicProtobufManager::protobuf_shutdown();

    dlclose(dl_handle);
    return 0;
}
