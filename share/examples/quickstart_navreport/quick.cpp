#include "dccl.h"
#include "navreport.pb.h"
#include <iostream>

int main()
{
    std::string encoded_bytes;
    dccl::Codec codec;
    codec.load<NavigationReport>();
    // SENDER
    {
        NavigationReport r_out;
        r_out.set_x(450);
        r_out.set_y(550);
        r_out.set_z(-100);
        r_out.set_veh_class(NavigationReport::AUV);
        r_out.set_battery_ok(true);
        
        codec.encode(&encoded_bytes, r_out);
    }
    // send encoded_bytes across your link

    // RECEIVER
    if(codec.id(encoded_bytes) == codec.id<NavigationReport>())
    {
        NavigationReport r_in;
        codec.decode(encoded_bytes, &r_in);
        std::cout << r_in.ShortDebugString() << std::endl;
    }    
}
