// Copyright 2011-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
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
#include <google/protobuf/descriptor.h>

#include "../../codec.h"

int main(int argc, char* argv[])
{
    std::cerr << "**** analyze_dccl is deprecated. You should consider using the newer 'dccl' tool "
                 "instead (dccl --analyze -f some_dccl.proto -I /path/to/proto) **** "
              << std::endl;

    if (argc < 2)
    {
        std::cerr << "usage: analyze_dccl some_dccl.proto [include_path (0-n)]" << std::endl;
        exit(1);
    }

    dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);

    dccl::DynamicProtobufManager::enable_compilation();

    for (int i = 2; i < argc; ++i) dccl::DynamicProtobufManager::add_include_path(argv[i]);

    const google::protobuf::FileDescriptor* file_desc =
        dccl::DynamicProtobufManager::load_from_proto_file(argv[1]);

    dccl::Codec dccl;
    if (file_desc)
    {
        std::cout << "read in: " << argv[1] << std::endl;
        for (int i = 0, n = file_desc->message_type_count(); i < n; ++i)
        {
            const google::protobuf::Descriptor* desc = file_desc->message_type(i);

            if (desc)
            {
                try
                {
                    dccl.load(desc);
                }
                catch (std::exception& e)
                {
                    std::cerr << "Not a valid DCCL message: " << desc->full_name() << "\n"
                              << e.what() << std::endl;
                }
            }
            else
            {
                std::cerr << "No descriptor with name " << file_desc->message_type(i)->full_name()
                          << " found!" << std::endl;
                exit(1);
            }
        }
        std::cout << dccl << std::endl;
    }
    else
    {
        std::cerr << "failed to read in: " << argv[1] << std::endl;
    }
}
