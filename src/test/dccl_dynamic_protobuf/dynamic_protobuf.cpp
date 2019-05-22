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

#include "dccl/dynamic_protobuf_manager.h"
#include <cassert>
#include <dlfcn.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <iostream>

#include "test_a.pb.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/libtest_dyn_protobuf"
              << std::endl;
    exit(1);
  }

  void *dl_handle = dlopen(argv[1], RTLD_LAZY);

  if (!dl_handle) {
    std::cerr << "Failed to open libtest_dyn_protobuf" SHARED_LIBRARY_SUFFIX
              << ", error: " << dlerror() << std::endl;
    exit(1);
  }

  boost::shared_ptr<google::protobuf::SimpleDescriptorDatabase> simple_database(
      new google::protobuf::SimpleDescriptorDatabase);
  dccl::DynamicProtobufManager::add_database(simple_database);

  {
    // testing compiled in
    boost::shared_ptr<google::protobuf::Message> adyn_msg =
        dccl::DynamicProtobufManager::new_protobuf_message("A");

    std::cout << adyn_msg->GetDescriptor()->DebugString() << std::endl;

    // testing dlopen'd
    boost::shared_ptr<google::protobuf::Message> bdyn_msg =
        dccl::DynamicProtobufManager::new_protobuf_message("B");

    std::cout << bdyn_msg->GetDescriptor()->DebugString() << std::endl;

    // test non-existent
    try {
      boost::shared_ptr<google::protobuf::Message> cdyn_msg =
          dccl::DynamicProtobufManager::new_protobuf_message("C");
      // should throw
      assert(false);
    } catch (std::exception &e) {
      // expected
    }

    // test dynamically loaded
    google::protobuf::FileDescriptorProto d_proto;
    std::string d_proto_str =
        "name: \"goby/test/util/dynamic_protobuf/test_d.proto\" "
        "message_type {   name: \"D\"   field {     name: \"d1\"     "
        "number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";

    google::protobuf::TextFormat::ParseFromString(d_proto_str, &d_proto);
    dccl::DynamicProtobufManager::add_protobuf_file(d_proto);

    boost::shared_ptr<google::protobuf::Message> ddyn_msg =
        dccl::DynamicProtobufManager::new_protobuf_message("D");

    std::cout << ddyn_msg->GetDescriptor()->DebugString() << std::endl;

    // test dynamically via separate database
    google::protobuf::FileDescriptorProto e_proto;
    std::string e_proto_str =
        "name: \"goby/test/util/dynamic_protobuf/test_e.proto\" "
        "message_type {   name: \"E\"   field {     name: \"e1\"     "
        "number: 1     label: LABEL_REQUIRED     type: TYPE_DOUBLE  } } ";

    google::protobuf::TextFormat::ParseFromString(e_proto_str, &e_proto);

    simple_database->Add(e_proto);

    boost::shared_ptr<google::protobuf::Message> edyn_msg =
        dccl::DynamicProtobufManager::new_protobuf_message("E");
    std::cout << edyn_msg->GetDescriptor()->DebugString() << std::endl;

    std::cout << "all tests passed" << std::endl;
  }

  dccl::DynamicProtobufManager::protobuf_shutdown();

  dlclose(dl_handle);
  return 0;
}
