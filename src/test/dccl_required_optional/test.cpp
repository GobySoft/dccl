// tests required versus optional encoding of fields using a presence bit

#include "dccl/codec.h"
#include "test.pb.h"
using namespace dccl::test;

using dccl::operator<<;

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);
    
    dccl::Codec codec;
    
    codec.load<BytesMsg>();
    codec.info<BytesMsg>(&dccl::dlog);

    BytesMsg msg_in;

    msg_in.set_req_bytes(dccl::hex_decode("88abcd1122338754"));
    msg_in.set_opt_bytes(dccl::hex_decode("102030adef2cb79d"));

    std::string encoded;
    codec.encode(&encoded, msg_in);
    
    BytesMsg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    
    
    std::cout << "all tests passed" << std::endl;
}

