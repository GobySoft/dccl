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
#include "dccl/cli_option.h"

enum Action { NO_ACTION, ENCODE, DECODE, ANALYZE };
enum Format { BIN, HEX, BASE64 };

struct Config
{
    Config()
        : action(NO_ACTION),
          format(BIN)
        { }
    
    Action action;
    std::set<std::string> include;
    std::set<std::string> dlopen;
    std::set<std::string> message;
    std::set<std::string> proto_file;
    Format format;
};

    
void analyze(dccl::Codec& dccl, const Config& cfg);
void encode(dccl::Codec& dccl, const Config& cfg);
void decode(dccl::Codec& dccl, const Config& cfg);

        
void load_desc(dccl::Codec* dccl,  const google::protobuf::Descriptor* desc, const std::string& name);
void parse_options(int argc, char* argv[], Config* cfg);

int main(int argc, char* argv[])
{
    Config cfg;
    parse_options(argc, argv, &cfg);

    dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);

    dccl::DynamicProtobufManager::enable_compilation();
    
    for(std::set<std::string>::const_iterator it = cfg.include.begin(),
            end = cfg.include.end(); it != end; ++it)
        dccl::DynamicProtobufManager::add_include_path(*it);

    dccl::Codec dccl;
    for(std::set<std::string>::const_iterator it = cfg.dlopen.begin(),
            end = cfg.dlopen.end(); it != end; ++it)
        dccl.load_library(*it);
    
    for(std::set<std::string>::const_iterator it = cfg.proto_file.begin(),
            end = cfg.proto_file.end(); it != end; ++it)
    {
        const google::protobuf::FileDescriptor* file_desc =
            dccl::DynamicProtobufManager::load_from_proto_file(*it);

        if(!file_desc)
        {
            std::cerr << "failed to read in: " << *it << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // if no messages explicity specified, load them all.
        if(cfg.message.empty())
        {
            for(int i = 0, n = file_desc->message_type_count(); i < n; ++i)
            {
                const google::protobuf::Descriptor* desc = file_desc->message_type(i);
                load_desc(&dccl, desc, file_desc->message_type(i)->full_name());
            }
        }
    }

    // Load up all the messages
    for(std::set<std::string>::const_iterator it = cfg.message.begin(),
            end = cfg.message.end(); it != end; ++it)   
    {
        const google::protobuf::Descriptor* desc = 
            dccl::DynamicProtobufManager::find_descriptor(*it);
        load_desc(&dccl, desc, *it);
    }

    switch(cfg.action)
    {
        case ENCODE: encode(dccl, cfg); break;
        case DECODE: decode(dccl, cfg); break;
        case ANALYZE: analyze(dccl, cfg); break;
        default:
            std::cout << "No action specified (e.g. analyze, decode, encode). Try --help." << std::endl;
            exit(EXIT_SUCCESS);
        
    }
}

void analyze(dccl::Codec& dccl, const Config& cfg)
{
    std::cout << dccl << std::endl;
}

void encode(dccl::Codec& dccl, const Config& cfg)
{
}

void decode(dccl::Codec& dccl, const Config& cfg)
{
}




void load_desc(dccl::Codec* dccl,  const google::protobuf::Descriptor* desc, const std::string& name)
{
    if(desc)
    {
        try { dccl->load(desc); }
        catch(std::exception& e)
        {
            std::cerr << "Not a valid DCCL message: " << desc->full_name() << "\nWhy: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "No descriptor with name " << name << " found! Make sure you have loaded all the necessary .proto files and/or shared libraries. Try --help." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void parse_options(int argc, char* argv[], Config* cfg)
{
    std::vector<dccl::Option> options;
    options.push_back(dccl::Option('e', "encode", no_argument, "Encode a DCCL message to STDOUT from STDIN"));
    options.push_back(dccl::Option('d', "decode", no_argument, "Decode a DCCL message to STDOUT from STDIN"));
    options.push_back(dccl::Option('a', "analyze", no_argument, "Provides information on a given DCCL message definition (e.g. field sizes)"));
    options.push_back(dccl::Option('h', "help", no_argument, "Gives help on the usage of 'dccl'"));
    options.push_back(dccl::Option('I', "proto_path", required_argument, "Add another search directory for .proto files"));
    options.push_back(dccl::Option('l', "dlopen", required_argument, "Open this shared library containing compiled DCCL messages."));
    options.push_back(dccl::Option('m', "message", required_argument, "Message name to encode, decode or analyze."));
    options.push_back(dccl::Option('f', "proto_file", required_argument, ".proto file to load."));
    options.push_back(dccl::Option(0, "format", required_argument, "Format for encode output or decode input: 'bin' (default) is a byte string, 'hex' is ascii-encoded hexadecimal, 'base64' is ascii-encoded base 64."));
    
    std::vector<option> long_options; 
    std::string opt_string;
    dccl::Option::convert_vector(options, &long_options, &opt_string);
    
    while (1) {
        int option_index = 0;

        int c = getopt_long(argc, argv, opt_string.c_str(),
                            &long_options[0], &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                // If this option set a flag, do nothing else now. 
                if (long_options[option_index].flag != 0)
                    break;

                if(!strcmp(long_options[option_index].name, "format"))
                {
                    if(!strcmp(optarg, "bin"))
                        cfg->format = BIN;
                    else if(!strcmp(optarg, "hex"))
                        cfg->format = HEX;
                    else if(!strcmp(optarg, "base64"))
                        cfg->format = BASE64;
                    else
                    {
                        std::cout << "Invalid format '" << optarg << "'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cout << "Try --help for valid options." << std::endl;
                    exit(EXIT_FAILURE);
                }
                
                break;
                
            case 'e': cfg->action = ENCODE; break;
            case 'd': cfg->action = DECODE; break;
            case 'a': cfg->action = ANALYZE; break;    
            case 'I': cfg->include.insert(optarg); break;
            case 'l': cfg->dlopen.insert(optarg); break;
            case 'm': cfg->message.insert(optarg); break;
            case 'f': cfg->proto_file.insert(optarg); break;                
                
            case 'h':
                std::cout << "Usage of the Dynamic Compact Control Language (DCCL) tool ('dccl'): " << std::endl;
                for(int i = 0, n = options.size(); i < n; ++i)
                    std::cout << "  " << options[i].usage() << std::endl;
                exit(EXIT_SUCCESS);
                break;
                
            case '?':
                std::cout << "Try --help for valid options." << std::endl;
                exit(EXIT_FAILURE);
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

