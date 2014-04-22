// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
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



// tests functionality of std::list<const google::protobuf::Message*> calls


#include "dccl/codec.h"
#include "dccl/field_codec_default.h"
#include "dccl/binary.h"

#include "test.pb.h"

using dccl::operator<<;

int main(int argc, char* argv[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    GobyMessage1 msg_in1;
    GobyMessage2 msg_in2;
    GobyMessage3 msg_in3;
    GobyMessage3 msg_in4;

    msg_in1.set_int32_val(1);
    msg_in2.set_bool_val(false);
    msg_in3.set_string_val("string1");
    msg_in4.set_string_val("string2");
    
    std::list<const google::protobuf::Message*> msgs;
    msgs.push_back(&msg_in1);
    msgs.push_back(&msg_in2);
    msgs.push_back(&msg_in3);    
    msgs.push_back(&msg_in4);

    std::list<const google::protobuf::Descriptor*> descs;
    descs.push_back(msg_in1.GetDescriptor());
    descs.push_back(msg_in2.GetDescriptor());
    descs.push_back(msg_in3.GetDescriptor());    
    descs.push_back(msg_in4.GetDescriptor());
    
    for(std::list<const google::protobuf::Descriptor*>::const_iterator it = descs.begin(),
            end = descs.end(); it != end; ++it)
    {
        codec.info(*it, &std::cout);
        codec.load(*it);
    }
    
    std::string bytes1;
    for(std::list<const google::protobuf::Message*>::const_iterator it = msgs.begin(),
            end = msgs.end(); it != end; ++it)
    {
        static int i = 0;
        std::cout << "Message " << ++i << " in:\n" << (*it)->DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        codec.encode(&bytes1, *(*it));
    }    
    bytes1 += std::string(4, '\0');    

    
    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;


    
    std::list< boost::shared_ptr<google::protobuf::Message> > msgs_out;
    try
    {
        while(!bytes1.empty())
            msgs_out.push_back(codec.decode<boost::shared_ptr<google::protobuf::Message> >(&bytes1));
    }
    catch(dccl::Exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    std::list<const google::protobuf::Message*>::const_iterator in_it = msgs.begin();

    assert(msgs.size() == msgs_out.size());
    
    for(std::list< boost::shared_ptr<google::protobuf::Message> >::const_iterator it = msgs_out.begin(),
            end = msgs_out.end(); it != end; ++it)
    {
        static int i = 0;
        std::cout << "... got Message " << ++i << " out:\n" << (*it)->DebugString() << std::endl;
        assert((*in_it)->SerializeAsString() == (*it)->SerializeAsString());
        ++in_it;
    }
    

    std::cout << "all tests passed" << std::endl;
}

