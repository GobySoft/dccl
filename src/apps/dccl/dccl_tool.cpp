// Copyright 2014-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   philboske <philboske@gmail.com>
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
//
// For the 'dccl' tool: loading non-GPL shared libraries for the purpose of
// using this tool does *not* violate the GPL license terms of DCCL.
//

#include <fstream>
#include <sstream>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>

#include "../../binary.h"
#include "../../cli_option.h"
#include "../../codec.h"

#include "dccl/version.h"
#include "dccl_tool.pb.h"

// for realpath
#include <climits>
#include <cstdlib>

// replacement for boost::trim_if
void trim_if(std::string& s, bool (*predicate)(char))
{
    // Trim from the start
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [predicate](char ch) { return !predicate(ch); }));

    // Trim from the end
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [predicate](char ch) { return !predicate(ch); }).base(),
        s.end());
}
void trim(std::string& s)
{
    trim_if(s, [](char ch) -> bool { return std::isspace(ch); });
}
std::string trim_copy(const std::string& s)
{
    auto cs = s;
    trim(cs);
    return cs;
}

enum Action
{
    NO_ACTION,
    ENCODE,
    DECODE,
    ANALYZE,
    DISP_PROTO
};
enum Format
{
    BINARY,
    TEXTFORMAT,
    HEX,
    BASE64
};

namespace dccl
{
/// 'dccl' command line tool namespace
namespace tool
{
struct Config
{
    Config() : id_codec(dccl::Codec::default_id_codec_name()) {}

    Action action{NO_ACTION};
    std::set<std::string> include;
    std::vector<std::string> dlopen;
    std::set<std::string> message;
    std::map<std::string, std::size_t> hash;
    std::set<std::string> proto_file;
    Format format{BINARY};
    std::string id_codec;
    bool verbose{false};
    bool omit_prefix{false};
    bool hash_only{false};
};
} // namespace tool
} // namespace dccl

void analyze(dccl::Codec& dccl, const dccl::tool::Config& cfg);
void encode(dccl::Codec& dccl, dccl::tool::Config& cfg);
void decode(dccl::Codec& dccl, const dccl::tool::Config& cfg);
void disp_proto(dccl::Codec& dccl, const dccl::tool::Config& cfg);

// return { success, hash }
std::pair<bool, std::size_t> load_desc(dccl::Codec* dccl, const google::protobuf::Descriptor* desc,
                                       const std::string& name);
void parse_options(int argc, char* argv[], dccl::tool::Config* cfg, int& console_width_);

int main(int argc, char* argv[])
{
    {
        dccl::tool::Config cfg;
        int console_width = -1;
        parse_options(argc, argv, &cfg, console_width);

        if (!cfg.verbose)
            dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);
        else
            dccl::dlog.connect(dccl::logger::DEBUG1_PLUS, &std::cerr);

        dccl::DynamicProtobufManager::enable_compilation();

        for (const auto& it : cfg.include) dccl::DynamicProtobufManager::add_include_path(it);

        std::string first_dl;
        if (cfg.dlopen.size())
            first_dl = cfg.dlopen[0];

        dccl::Codec dccl(cfg.id_codec, first_dl);

        if (console_width >= 0)
        {
            dccl.set_console_width(console_width);
        }

        if (cfg.dlopen.size() > 1)
        {
            for (auto it = cfg.dlopen.begin() + 1, n = cfg.dlopen.end(); it != n; ++it)
                dccl.load_library(*it);
        }
        bool no_messages_specified = cfg.message.empty();
        for (auto it = cfg.proto_file.begin(), end = cfg.proto_file.end(); it != end; ++it)
        {
            const google::protobuf::FileDescriptor* file_desc =
                dccl::DynamicProtobufManager::load_from_proto_file(*it);

            if (!file_desc)
            {
                std::cerr << "failed to read in: " << *it << std::endl;
                exit(EXIT_FAILURE);
            }

            // if no messages explicitly specified, load them all.
            if (no_messages_specified)
            {
                for (int i = 0, n = file_desc->message_type_count(); i < n; ++i)
                {
                    cfg.message.insert(file_desc->message_type(i)->full_name());
                    if (i == 0 && cfg.action == ENCODE)
                    {
                        std::cerr << "Encoding assuming message: "
                                  << file_desc->message_type(i)->full_name() << std::endl;
                        break;
                    }
                }
            }
        }

        // Load up all the messages
        for (auto it = cfg.message.begin(); it != cfg.message.end();)
        {
            const google::protobuf::Descriptor* desc =
                dccl::DynamicProtobufManager::find_descriptor(*it);
            // if we can't load the message, erase it from our set of messages
            bool success;
            std::size_t hash;
            std::tie(success, hash) = load_desc(&dccl, desc, *it);
            if (!success)
            {
                it = cfg.message.erase(it);
            }
            else
            {
                cfg.hash.insert(std::make_pair(*it, hash));
                ++it;
            }
        }

        switch (cfg.action)
        {
            case ENCODE: encode(dccl, cfg); break;
            case DECODE: decode(dccl, cfg); break;
            case ANALYZE: analyze(dccl, cfg); break;
            case DISP_PROTO: disp_proto(dccl, cfg); break;
            default:
                std::cerr << "No action specified (e.g. analyze, decode, encode). Try --help."
                          << std::endl;
                exit(EXIT_SUCCESS);
        }
    }
}

void analyze(dccl::Codec& codec, const dccl::tool::Config& cfg)
{
    for (const auto& name : cfg.message)
    {
        if (!cfg.hash_only)
        {
            const google::protobuf::Descriptor* desc =
                dccl::DynamicProtobufManager::find_descriptor(name);
            codec.info(desc, &std::cout);
        }
        else
        {
            // only write name when providing hashes for multiple messages
            if (cfg.message.size() > 1)
                std::cout << name << ": ";
            std::cout << dccl::hash_as_string(cfg.hash.at(name)) << std::endl;
        }
    }
}

void encode(dccl::Codec& dccl, dccl::tool::Config& cfg)
{
    if (cfg.message.size() > 1)
    {
        std::cerr << "No more than one DCCL message can be specified with -m or --message for "
                     "encoding."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (cfg.message.size() == 0)
    {
        std::cerr << "You must specify a DCCL message to encode with -m" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string command_line_name = *cfg.message.begin();

    while (!std::cin.eof())
    {
        std::string input;
        std::getline(std::cin, input);

        trim(input);
        if (input.empty())
            continue;

        std::string name;
        if (input[0] == '|')
        {
            std::string::size_type close_bracket_pos = input.find('|', 1);
            if (close_bracket_pos == std::string::npos)
            {
                std::cerr << "Incorrectly formatted input: expected '|'" << std::endl;
                exit(EXIT_FAILURE);
            }

            name = input.substr(1, close_bracket_pos - 1);
            if (cfg.message.find(name) == cfg.message.end())
            {
                const google::protobuf::Descriptor* desc =
                    dccl::DynamicProtobufManager::find_descriptor(name);
                if (!load_desc(&dccl, desc, name).first)
                {
                    std::cerr << "Could not load descriptor for message " << name << std::endl;
                    exit(EXIT_FAILURE);
                }

                cfg.message.insert(name);
            }

            if (input.size() > close_bracket_pos + 1)
                input = input.substr(close_bracket_pos + 1);
            else
                input.clear();
        }
        else
        {
            if (cfg.message.size() == 0)
            {
                std::cerr << "Message name not given with -m or in the input (i.e. '[Name] field1: "
                             "value field2: value')."
                          << std::endl;
                exit(EXIT_FAILURE);
            }

            name = command_line_name;
        }

        const google::protobuf::Descriptor* desc =
            dccl::DynamicProtobufManager::find_descriptor(name);
        if (desc == nullptr)
        {
            std::cerr << "No descriptor with name " << name
                      << " found! Make sure you have loaded all the necessary .proto files and/or "
                         "shared libraries. Also make sure you specified the fully qualified name "
                         "including the package, if any (e.g. 'goby.acomms.protobuf.NetworkAck', "
                         "not just 'NetworkAck')."
                      << std::endl;
            exit(EXIT_FAILURE);
        }

        std::shared_ptr<google::protobuf::Message> msg =
            dccl::DynamicProtobufManager::new_protobuf_message(desc);
        google::protobuf::TextFormat::ParseFromString(input, msg.get());

        if (msg->IsInitialized())
        {
            std::string encoded;
            dccl.encode(&encoded, *msg);
            switch (cfg.format)
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
                    google::protobuf::TextFormat::PrintFieldValueToString(
                        s, s.GetDescriptor()->FindFieldByNumber(1), -1, &output);

                    std::cout << output << std::endl;
                    break;
                }

                case HEX: std::cout << dccl::hex_encode(encoded) << std::endl; break;
                case BASE64:
#if DCCL_HAS_B64
                    std::stringstream instream(encoded);
                    std::stringstream outstream;
                    ::base64::encoder D;
                    D.encode(instream, outstream);
                    std::cout << outstream.str();
#else
                    std::cerr << "dccl was not compiled with libb64-dev, so no Base64 "
                                 "functionality is available."
                              << std::endl;
                    exit(EXIT_FAILURE);
#endif
                    break;
            }
        }
    }
}

void decode(dccl::Codec& dccl, const dccl::tool::Config& cfg)
{
    std::string input;
    if (cfg.format == BINARY)
    {
        std::ifstream fin("/dev/stdin", std::ios::binary);
        std::ostringstream ostrm;
        ostrm << fin.rdbuf();
        input = ostrm.str();
    }
    else
    {
        while (!std::cin.eof())
        {
            std::string line;
            std::getline(std::cin, line);

            if (trim_copy(line).empty())
                continue;

            switch (cfg.format)
            {
                default:
                case BINARY: break;

                case TEXTFORMAT:
                {
                    trim_if(line, [](char ch) -> bool { return ch == '"'; });

                    dccl::tool::protobuf::ByteString s;
                    google::protobuf::TextFormat::ParseFieldValueFromString(
                        "\"" + line + "\"", s.GetDescriptor()->FindFieldByNumber(1), &s);
                    input += s.b();
                    break;
                }
                case HEX: input += dccl::hex_decode(line); break;
                case BASE64:
#if DCCL_HAS_B64
                    std::stringstream instream(line);
                    std::stringstream outstream;
                    ::base64::decoder D;
                    D.decode(instream, outstream);
                    input += outstream.str();
                    break;
#else
                    std::cerr << "dccl was not compiled with libb64-dev, so no Base64 "
                                 "functionality is available."
                              << std::endl;
                    exit(EXIT_FAILURE);
#endif
            }
        }
    }

    while (!input.empty())
    {
        std::shared_ptr<google::protobuf::Message> msg =
            dccl.decode<std::shared_ptr<google::protobuf::Message>>(&input);
        if (!cfg.omit_prefix)
            std::cout << "|" << msg->GetDescriptor()->full_name() << "| ";
        std::cout << msg->ShortDebugString() << std::endl;
    }
}

void disp_proto(dccl::Codec& /*dccl*/, const dccl::tool::Config& cfg)
{
    std::cout << "Please note that for Google Protobuf versions < 2.5.0, the dccl extensions will "
                 "not be show below, so you'll need to refer to the original .proto file."
              << std::endl;
    for (const auto& it : cfg.message)
    {
        const google::protobuf::Descriptor* desc =
            dccl::DynamicProtobufManager::find_descriptor(it);

        std::cout << desc->DebugString();
    }
}

std::pair<bool, std::size_t> load_desc(dccl::Codec* dccl, const google::protobuf::Descriptor* desc,
                                       const std::string& name)
{
    if (desc)
    {
        try
        {
            std::size_t hash = dccl->load(desc);
            return {true, hash};
        }
        catch (std::exception& e)
        {
            std::cerr << "Not a valid DCCL message: " << desc->full_name()
                      << "\n\tWhy: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "No descriptor with name " << name
                  << " found! Make sure you have loaded all the necessary .proto files and/or "
                     "shared libraries. Try --help."
                  << std::endl;
    }
    return {false, 0};
}

void parse_options(int argc, char* argv[], dccl::tool::Config* cfg, int& console_width_)
{
    std::vector<dccl::Option> options;
    options.emplace_back('e', "encode", no_argument, "Encode a DCCL message to STDOUT from STDIN");
    options.emplace_back('d', "decode", no_argument, "Decode a DCCL message to STDOUT from STDIN");
    options.emplace_back(
        'a', "analyze", no_argument,
        "Provides information on a given DCCL message definition (e.g. field sizes)");
    options.emplace_back('p', "display_proto", no_argument,
                         "Display the .proto definition of this message.");
    options.emplace_back('h', "help", no_argument, "Gives help on the usage of 'dccl'");
    options.emplace_back('I', "proto_path", required_argument,
                         "Add another search directory for .proto files");
    options.emplace_back('l', "dlopen", required_argument,
                         "Open this shared library containing compiled DCCL messages.");
    options.emplace_back('m', "message", required_argument,
                         "Message name to encode, decode or analyze.");
    options.emplace_back('f', "proto_file", required_argument, ".proto file to load.");
    options.emplace_back(0, "format", required_argument,
                         "Format for encode output or decode input: 'bin' (default) is raw binary, "
                         "'hex' is ascii-encoded hexadecimal, 'textformat' is a Google Protobuf "
                         "TextFormat byte string, 'base64' is ascii-encoded base 64.");
    options.emplace_back('v', "verbose", no_argument, "Display extra debugging information.");
    options.emplace_back('o', "omit_prefix", no_argument,
                         "Omit the DCCL type name prefix from the output of decode.");
    options.emplace_back('i', "id_codec", required_argument,
                         "(Advanced) name for a nonstandard DCCL ID codec to use");
    options.emplace_back('V', "version", no_argument, "DCCL Version");
    options.emplace_back('w', "console_width", required_argument,
                         "Maximum number of characters used for prettifying console outputs.");
    options.emplace_back('H', "hash_only", no_argument, "Only display hash for --analyze action.");

    std::vector<option> long_options;
    std::string opt_string;
    dccl::Option::convert_vector(options, &long_options, &opt_string);

    while (1)
    {
        int option_index = 0;

        int c = getopt_long(argc, argv, opt_string.c_str(), &long_options[0], &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                // If this option set a flag, do nothing else now.
                if (long_options[option_index].flag != nullptr)
                    break;

                if (!strcmp(long_options[option_index].name, "format"))
                {
                    if (!strcmp(optarg, "textformat"))
                        cfg->format = TEXTFORMAT;
                    else if (!strcmp(optarg, "hex"))
                        cfg->format = HEX;
                    else if (!strcmp(optarg, "base64"))
                        cfg->format = BASE64;
                    else if (!strcmp(optarg, "bin"))
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
            case 'l': cfg->dlopen.emplace_back(optarg); break;
            case 'm': cfg->message.insert(optarg); break;
            case 'f':
            {
                char* proto_file_canonical_path = realpath(optarg, nullptr);
                if (proto_file_canonical_path)
                {
                    cfg->proto_file.insert(proto_file_canonical_path);
                    free(proto_file_canonical_path);
                }
                else
                {
                    std::cerr << "Invalid proto file path: '" << optarg << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'i': cfg->id_codec = optarg; break;
            case 'v': cfg->verbose = true; break;
            case 'o': cfg->omit_prefix = true; break;
            case 'H': cfg->hash_only = true; break;

            case 'h':
                std::cout << "Usage of the Dynamic Compact Control Language (DCCL) tool ('dccl'): "
                          << std::endl;
                for (auto& option : options) std::cout << "  " << option.usage() << std::endl;
                exit(EXIT_SUCCESS);
                break;

            case 'V':
                std::cout << dccl::VERSION_STRING << std::endl;
                exit(EXIT_SUCCESS);
                break;

            case 'w':
            {
                // Robust error checking inspired by https://stackoverflow.com/a/26083517.
                char* end_ptr = nullptr;
                errno = 0;
                auto number = strtol(optarg, &end_ptr, 10);

                if (optarg == end_ptr)
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (no digits found, 0 returned)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if ((errno == ERANGE) && (number == LONG_MIN))
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (underflow occurred)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if ((errno == ERANGE) && (number == LONG_MAX))
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (overflow occurred)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if (errno == EINVAL)
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (base contains unsupported value)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if ((errno != 0) && (number == 0))
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (unspecified error occurred)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if ((errno == 0) && optarg && (*end_ptr != 0))
                {
                    std::cerr << "Option -w value \'" << optarg
                              << "\' was invalid (contains additional characters)." << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if ((errno == 0) && optarg && !*end_ptr)
                {
                    if (number >= 0)
                    {
                        console_width_ = number;
                    }
                    else
                    {
                        std::cerr << "Option -w value \'" << optarg
                                  << "\' was invalid (negative number)." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "Option -w value \'" << optarg << "\' was invalid (unknown error)."
                              << std::endl;
                    exit(EXIT_FAILURE);
                }

                break;
            }

            case '?': std::cerr << "Try --help for valid options." << std::endl; exit(EXIT_FAILURE);
            default: exit(EXIT_FAILURE);
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        std::cerr << "Unknown arguments: \n";
        while (optind < argc) std::cerr << argv[optind++];
        std::cerr << std::endl;
        std::cerr << "Try --help for valid options." << std::endl;
        exit(EXIT_FAILURE);
    }
}
