#include <google/protobuf/descriptor.h>

#include "dccl/codec.h"



int main(int argc, char* argv[])
{
    std::cerr << "**** analyze_dccl is deprecated. You should consider using the newer 'dccl' tool instead (dccl --analyze -f some_dccl.proto -I /path/to/proto) **** " << std::endl;
    
    if(argc < 2)
    {
        std::cerr << "usage: analyze_dccl some_dccl.proto [include_path (0-n)]" << std::endl;
        exit(1);
    }

    dccl::dlog.connect(dccl::logger::WARN_PLUS, &std::cerr);

    dccl::DynamicProtobufManager::enable_compilation();
    
    for(int i = 2; i < argc; ++i)
        dccl::DynamicProtobufManager::add_include_path(argv[i]);
    
    const google::protobuf::FileDescriptor* file_desc =
        dccl::DynamicProtobufManager::load_from_proto_file(argv[1]);

    
    dccl::Codec dccl;
    if(file_desc)
    {
        std::cout << "read in: " << argv[1] << std::endl;
        for(int i = 0, n = file_desc->message_type_count(); i < n; ++i)
        {
            const google::protobuf::Descriptor* desc = file_desc->message_type(i);
            
            if(desc)
            {
                try { dccl.load(desc); }
                catch(std::exception& e)
                {
                    std::cerr << "Not a valid DCCL message: " << desc->full_name() << "\n" << e.what() << std::endl;
                }
            }
            else
            {
                std::cerr << "No descriptor with name " <<
                    file_desc->message_type(i)->full_name() << " found!" << std::endl;
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
