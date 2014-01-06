// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Applications
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.


#include <google/protobuf/descriptor.h>

#include "dccl/codec.h"

#include <getopt.h>

void parse_options(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    parse_options(argc, argv);
    
    dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);

    // dccl::DynamicProtobufManager::enable_compilation();
    
    // for(int i = 2; i < argc; ++i)
    //     dccl::DynamicProtobufManager::add_include_path(argv[i]);
    
    // const google::protobuf::FileDescriptor* file_desc =
    //     dccl::DynamicProtobufManager::load_from_proto_file(argv[1]);

    
    // dccl::Codec dccl;
    // if(file_desc)
    // {
    //     std::cout << "read in: " << argv[1] << std::endl;
    //     for(int i = 0, n = file_desc->message_type_count(); i < n; ++i)
    //     {
    //         const google::protobuf::Descriptor* desc = file_desc->message_type(i);
            
    //         if(desc)
    //         {
    //             try { dccl.load(desc); }
    //             catch(std::exception& e)
    //             {
    //                 std::cerr << "Not a valid DCCL message: " << desc->full_name() << "\n" << e.what() << std::endl;
    //             }
    //         }
    //         else
    //         {
    //             std::cerr << "No descriptor with name " <<
    //                 file_desc->message_type(i)->full_name() << " found!" << std::endl;
    //             exit(1);
    //         }
    //     }
    //     std::cout << dccl << std::endl;
    // }
    // else
    // {
    //     std::cerr << "failed to read in: " << argv[1] << std::endl;
    // }    
}


void parse_options(int argc, char* argv[])
{
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"encode",  no_argument, 0,  'e' },
            {"decode",  no_argument, 0,  'd' },
            {"analyze", no_argument, 0,  'a' },
            {"proto_path", required_argument, 0, 'I' },
            {"dlopen", required_argument, 0, 'l' },
            {0,         0,           0,   0  }
        };

        int c = getopt_long(argc, argv, "edaI:d:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                // If this option set a flag, do nothing else now. 
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
                
            case 'e':
                printf("option encode\n");
                break;

            case 'd':
                printf("option decode\n");
                break;

            case 'a':
                printf("option analyze\n");
                break;
                
            case 'I':
                printf("option include with argument %s\n", optarg);
                break;
                
            case 'l':
                printf("option dlopen with argument %s\n", optarg);
                break;
                
            case '?': exit(EXIT_FAILURE);
            default: exit(EXIT_FAILURE);
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }
}

