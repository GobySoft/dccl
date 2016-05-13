//
// For the 'dccl' tool: loading non-GPL shared libraries for the purpose of
// using this tool does *not* violate the GPL license terms of DCCL.
//



#include <sstream>
#include <fstream>


#include <google/protobuf/descriptor.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/descriptor.pb.h>

#include <boost/algorithm/string.hpp>

#include "dccl/codec.h"
#include "dccl/cli_option.h"
#include "dccl/b64/encode.h"
#include "dccl/b64/decode.h"
#include "dccl_tool.pb.h"
#include "dccl/version.h"

enum Action { NO_ACTION, ENCODE, DECODE, ANALYZE, DISP_PROTO };
enum Format { BINARY, TEXTFORMAT, HEX, BASE64 };

namespace dccl
{
    /// 'dccl' command line tool namespace
    namespace tool
    {
        struct Config
        {
            Config()
                : action(NO_ACTION),
                  format(BINARY),
                  id_codec(dccl::Codec::default_id_codec_name()),
                  verbose(false),
                  omit_prefix(false)
                { }
    
            Action action;
            std::set<std::string> include;
            std::vector<std::string> dlopen;
            std::set<std::string> message;
            std::set<std::string> proto_file;
            Format format;
            std::string id_codec;
            bool verbose;
            bool omit_prefix;
        };
    }
}

    
void analyze(dccl::Codec& dccl, const dccl::tool::Config& cfg);
void encode(dccl::Codec& dccl, dccl::tool::Config& cfg);
void decode(dccl::Codec& dccl, const dccl::tool::Config& cfg);
void disp_proto(dccl::Codec& dccl, const dccl::tool::Config& cfg);

        
void load_desc(dccl::Codec* dccl,  const google::protobuf::Descriptor* desc, const std::string& name);
void parse_options(int argc, char* argv[], dccl::tool::Config* cfg);


int main(int argc, char* argv[])
{
    {
        dccl::tool::Config cfg;
        parse_options(argc, argv, &cfg);

        if(!cfg.verbose)
            dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);
        else
            dccl::dlog.connect(dccl::logger::DEBUG1_PLUS, &std::cerr);
        
        dccl::DynamicProtobufManager::enable_compilation();
    
        for(std::set<std::string>::const_iterator it = cfg.include.begin(),
                end = cfg.include.end(); it != end; ++it)
            dccl::DynamicProtobufManager::add_include_path(*it);
    


        std::string first_dl;
        if(cfg.dlopen.size())
            first_dl = cfg.dlopen[0];
        
        dccl::Codec dccl(cfg.id_codec, first_dl);

        if(cfg.dlopen.size() > 1)
        {
            for(std::vector<std::string>::iterator it = cfg.dlopen.begin()+1,
                    n = cfg.dlopen.end(); it != n; ++it)
                dccl.load_library(*it);
        }
        bool no_messages_specified = cfg.message.empty();
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
        
            // if no messages explicitly specified, load them all.
            if(no_messages_specified)
            {
                for(int i = 0, n = file_desc->message_type_count(); i < n; ++i)
                {
                    cfg.message.insert(file_desc->message_type(i)->full_name());
                    if(i == 0 && cfg.action == ENCODE)
                    {
                        std::cerr << "Encoding assuming message: " << file_desc->message_type(i)->full_name() << std::endl;
                        break;
                    }
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
            case DISP_PROTO: disp_proto(dccl, cfg); break;
            default:
                std::cerr << "No action specified (e.g. analyze, decode, encode). Try --help." << std::endl;
                exit(EXIT_SUCCESS);
        
        }
    }    
}



void analyze(dccl::Codec& dccl, const dccl::tool::Config& cfg)
{
    dccl.info_all(&std::cout);
}

void encode(dccl::Codec& dccl, dccl::tool::Config& cfg)
{
    if(cfg.message.size() > 1)
    {
        std::cerr << "No more than one DCCL message can be specified with -m or --message for encoding." << std::endl;
        exit(EXIT_FAILURE);
    }
    else if(cfg.message.size() == 0)
    {
        std::cerr << "You must specify a DCCL message to encode with -m" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::string command_line_name = *cfg.message.begin();
    
    while(!std::cin.eof())
    {
        std::string input;
        std::getline (std::cin, input);

        boost::trim(input);
        if(input.empty())
            continue;

        std::string name;
        if(input[0] == '|')
        {
            std::string::size_type close_bracket_pos = input.find('|', 1);
            if(close_bracket_pos == std::string::npos)
            {
                std::cerr << "Incorrectly formatted input: expected '|'" << std::endl;
                exit(EXIT_FAILURE);
            }

            name = input.substr(1, close_bracket_pos-1);
            if(cfg.message.find(name) == cfg.message.end())
            {
                const google::protobuf::Descriptor* desc = 
                    dccl::DynamicProtobufManager::find_descriptor(name);
                load_desc(&dccl, desc, name);
                cfg.message.insert(name);
            }
            
            
            if(input.size() > close_bracket_pos+1)
                input = input.substr(close_bracket_pos+1);
            else
                input.clear();
        }
        else
        {
            if(cfg.message.size() == 0)
            {
                std::cerr << "Message name not given with -m or in the input (i.e. '[Name] field1: value field2: value')." << std::endl;
                exit(EXIT_FAILURE);
            }
                
            name = command_line_name;
        }
        
        const google::protobuf::Descriptor* desc = dccl::DynamicProtobufManager::find_descriptor(name);
        if(desc == 0)
        {
            std::cerr << "No descriptor with name " << name << " found! Make sure you have loaded all the necessary .proto files and/or shared libraries. Also make sure you specified the fully qualified name including the package, if any (e.g. 'goby.acomms.protobuf.NetworkAck', not just 'NetworkAck')." << std::endl;
            exit(EXIT_FAILURE);
        }
        
        
        boost::shared_ptr<google::protobuf::Message> msg = dccl::DynamicProtobufManager::new_protobuf_message(desc);
        google::protobuf::TextFormat::ParseFromString(input, msg.get());

        if(msg->IsInitialized())
        {
            std::string encoded;
            dccl.encode(&encoded, *msg);
            switch(cfg.format)
            {
                default:
                case BINARY:                    
                {    
                    std::ofstream fout("/dev/stdout", std::ios::binary | std::ios::app);
                    fout.write(encoded.data(), encoded.size()); 
                    break;
                }
                
                case TEXTFORMAT:
                {
                    
                    dccl::tool::protobuf::ByteString s;
                    s.set_b(encoded);
                    std::string output;
                    google::protobuf::TextFormat::PrintFieldValueToString(s, s.GetDescriptor()->FindFieldByNumber(1), -1, &output);

                    std::cout << output << std::endl;
                    break;
                }
                
                case HEX:
                    std::cout << dccl::hex_encode(encoded) << std::endl;
                    break;
                case BASE64:
                    std::stringstream instream(encoded);
                    std::stringstream outstream;
                    dccl::base64::encoder D;
                    D.encode(instream, outstream);
                    std::cout << outstream.str();
                    break;
            }
        
        }
    }    
}

void decode(dccl::Codec& dccl, const dccl::tool::Config& cfg)
{
    std::string input;
    if(cfg.format == BINARY)
    {
        std::ifstream fin("/dev/stdin", std::ios::binary);
        std::ostringstream ostrm;
        ostrm << fin.rdbuf();
        input = ostrm.str();
    }
    else
    {
        while(!std::cin.eof())
        {
            std::string line;
            std::getline (std::cin, line);
            
            if(boost::trim_copy(line).empty())
                continue;
            
            switch(cfg.format)
            {
                default:
                case BINARY:
                    break;
                    
                case TEXTFORMAT:
                {
                    boost::trim_if(line, boost::is_any_of("\""));
                    
                    
                    dccl::tool::protobuf::ByteString s;
                    google::protobuf::TextFormat::ParseFieldValueFromString("\"" + line + "\"", s.GetDescriptor()->FindFieldByNumber(1), &s);
                    input += s.b();
                    break;
                }
                case HEX:
                    input += dccl::hex_decode(line);
                    break;
                case BASE64:
                    std::stringstream instream(line);
                    std::stringstream outstream;
                    dccl::base64::decoder D;
                    D.decode(instream, outstream);
                    input += outstream.str();
                    break;
            }
        }
    }

    while(!input.empty())
    {
        boost::shared_ptr<google::protobuf::Message> msg = dccl.decode<boost::shared_ptr<google::protobuf::Message> >(&input);
        if(!cfg.omit_prefix)
            std::cout << "|" << msg->GetDescriptor()->full_name() << "| ";
        std::cout << msg->ShortDebugString() << std::endl;
    }
}

void disp_proto(dccl::Codec& dccl, const dccl::tool::Config& cfg)
{
    std::cout << "Please note that for Google Protobuf versions < 2.5.0, the dccl extensions will not be show below, so you'll need to refer to the original .proto file." << std::endl;
   for(std::set<std::string>::const_iterator it = cfg.message.begin(),
            end = cfg.message.end(); it != end; ++it)   
    {    
        const google::protobuf::Descriptor* desc = 
            dccl::DynamicProtobufManager::find_descriptor(*it);

        std::cout << desc->DebugString();
        
    }
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

void parse_options(int argc, char* argv[], dccl::tool::Config* cfg)
{
    std::vector<dccl::Option> options;
    options.push_back(dccl::Option('e', "encode", no_argument, "Encode a DCCL message to STDOUT from STDIN"));
    options.push_back(dccl::Option('d', "decode", no_argument, "Decode a DCCL message to STDOUT from STDIN"));
    options.push_back(dccl::Option('a', "analyze", no_argument, "Provides information on a given DCCL message definition (e.g. field sizes)"));
    options.push_back(dccl::Option('p', "display_proto", no_argument, "Display the .proto definition of this message."));
    options.push_back(dccl::Option('h', "help", no_argument, "Gives help on the usage of 'dccl'"));
    options.push_back(dccl::Option('I', "proto_path", required_argument, "Add another search directory for .proto files"));
    options.push_back(dccl::Option('l', "dlopen", required_argument, "Open this shared library containing compiled DCCL messages."));
    options.push_back(dccl::Option('m', "message", required_argument, "Message name to encode, decode or analyze."));
    options.push_back(dccl::Option('f', "proto_file", required_argument, ".proto file to load."));
    options.push_back(dccl::Option(0, "format", required_argument, "Format for encode output or decode input: 'bin' (default) is raw binary, 'hex' is ascii-encoded hexadecimal, 'textformat' is a Google Protobuf TextFormat byte string, 'base64' is ascii-encoded base 64."));
    options.push_back(dccl::Option('v', "verbose", no_argument, "Display extra debugging information."));
    options.push_back(dccl::Option('o', "omit_prefix", no_argument, "Omit the DCCL type name prefix from the output of decode."));
    options.push_back(dccl::Option('i', "id_codec", required_argument, "(Advanced) name for a nonstandard DCCL ID codec to use"));
    options.push_back(dccl::Option('V', "version", no_argument, "DCCL Version"));
    
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
                    if(!strcmp(optarg, "textformat"))
                        cfg->format = TEXTFORMAT;
                    else if(!strcmp(optarg, "hex"))
                        cfg->format = HEX;
                    else if(!strcmp(optarg, "base64"))
                        cfg->format = BASE64;
                    else if(!strcmp(optarg, "bin"))
                        cfg->format = BINARY;
                    else
                    {
                        std::cerr << "Invalid format '" << optarg << "'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Try --help for valid options." << std::endl;
                    exit(EXIT_FAILURE);
                }
                
                break;
                
            case 'e': cfg->action = ENCODE; break;
            case 'd': cfg->action = DECODE; break;
            case 'a': cfg->action = ANALYZE; break;    
            case 'p': cfg->action = DISP_PROTO; break;    
            case 'I': cfg->include.insert(optarg); break;
            case 'l': cfg->dlopen.push_back(optarg); break;
            case 'm': cfg->message.insert(optarg); break;
            case 'f': cfg->proto_file.insert(optarg); break;                
            case 'i': cfg->id_codec = optarg; break;                
            case 'v': cfg->verbose = true; break;                
            case 'o': cfg->omit_prefix = true; break;                
                
            case 'h':
                std::cout << "Usage of the Dynamic Compact Control Language (DCCL) tool ('dccl'): " << std::endl;
                for(int i = 0, n = options.size(); i < n; ++i)
                    std::cout << "  " << options[i].usage() << std::endl;
                exit(EXIT_SUCCESS);
                break;

            case 'V':
                std::cout << dccl::VERSION_STRING  << std::endl;
                exit(EXIT_SUCCESS);
                break;

                
            case '?':
                std::cerr << "Try --help for valid options." << std::endl;
                exit(EXIT_FAILURE);
            default: exit(EXIT_FAILURE);
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        std::cerr << "Unknown arguments: \n";
        while (optind < argc)
            std::cerr << argv[optind++];
        std::cerr << std::endl;
        std::cerr << "Try --help for valid options." << std::endl;
        exit(EXIT_FAILURE);
    }
}

