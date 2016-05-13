// tests receiving one of several static types

#include "dccl/codec.h"
#include "test.pb.h"

using namespace dccl::test;

using dccl::operator<<;

GobyMessage1 msg_in1;
GobyMessage2 msg_in2;
GobyMessage3 msg_in3;
dccl::Codec codec;

void decode(const std::string& bytes);

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    codec.load<GobyMessage1>();
    codec.load<GobyMessage2>();
    codec.load<GobyMessage3>();

    codec.info<GobyMessage1>(&dccl::dlog);
    codec.info<GobyMessage2>(&dccl::dlog);
    codec.info<GobyMessage3>(&dccl::dlog);

    
 
    msg_in1.set_int32_val(1);
    msg_in2.set_bool_val(false);
    msg_in3.set_string_val("string1");

    std::cout << "Try encode..." << std::endl;
    std::string bytes1, bytes2, bytes3;
    codec.encode(&bytes1, msg_in1);
    std::cout << "... got bytes for GobyMessage1 (hex): " << dccl::hex_encode(bytes1) << std::endl;
    codec.encode(&bytes2, msg_in2);
    std::cout << "... got bytes for GobyMessage2 (hex): " << dccl::hex_encode(bytes2) << std::endl;
    codec.encode(&bytes3, msg_in3);
    std::cout << "... got bytes for GobyMessage3 (hex): " << dccl::hex_encode(bytes3) << std::endl;

    std::cout << "Try decode..." << std::endl;

    // mix up the order
    decode(bytes2);
    decode(bytes1);
    decode(bytes3);
    

    std::cout << "all tests passed" << std::endl;
}

void decode(const std::string& bytes)
{
    unsigned dccl_id = codec.id(bytes);
    
    if(dccl_id == codec.id<GobyMessage1>())
    {
        GobyMessage1 msg_out1;
        codec.decode(bytes, &msg_out1);

        std::cout << "Got..." << msg_out1 << std::endl;
        assert(msg_out1.SerializeAsString() == msg_in1.SerializeAsString());
    }
    else if(dccl_id == codec.id<GobyMessage2>())
    {
        GobyMessage2 msg_out2;
        codec.decode(bytes, &msg_out2);

        std::cout << "Got..." << msg_out2 << std::endl;
        assert(msg_out2.SerializeAsString() == msg_in2.SerializeAsString());
    }
    else if(dccl_id == codec.id<GobyMessage3>())
    {
        GobyMessage3 msg_out3;
        codec.decode(bytes, &msg_out3);
        
        std::cout << "Got..." << msg_out3 << std::endl;
        assert(msg_out3.SerializeAsString() == msg_in3.SerializeAsString());
    }
    
}
