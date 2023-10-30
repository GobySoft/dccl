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
// tests functionality of std::list<const google::protobuf::Message*> calls

#include "../../binary.h"
#include "../../codec.h"

#include <list>

#include "test.pb.h"
using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/ [])
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

    for (auto desc : descs)
    {
        codec.info(desc, &std::cout);
        codec.load(desc);
    }

    std::string bytes1;
    for (auto msg : msgs)
    {
        static int i = 0;
        std::cout << "Message " << ++i << " in:\n" << msg->DebugString() << std::endl;
        std::cout << "Try encode..." << std::endl;
        codec.encode(&bytes1, *msg);
    }
    bytes1 += std::string(4, '\0');

    std::cout << "... got bytes (hex): " << dccl::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;

    // non-destructive
    {
        std::list<std::shared_ptr<google::protobuf::Message>> msgs_out;
        try
        {
            std::string::iterator begin = bytes1.begin(), end = bytes1.end();
            while (begin != end)
            {
                auto it = codec.loaded().find(codec.id(begin, end));
                if (it == codec.loaded().end())
                    break;

                std::shared_ptr<google::protobuf::Message> msg =
                    dccl::DynamicProtobufManager::new_protobuf_message(it->second);
                begin = codec.decode(begin, end, msg.get());
                msgs_out.push_back(msg);
            }
        }
        catch (dccl::Exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        std::list<const google::protobuf::Message*>::const_iterator in_it = msgs.begin();

        assert(msgs.size() == msgs_out.size());

        for (const auto& it : msgs_out)
        {
            static int i = 0;
            std::cout << "... got Message " << ++i << " out:\n" << it->DebugString() << std::endl;
            assert((*in_it)->SerializeAsString() == it->SerializeAsString());
            ++in_it;
        }
    }

    // destructive
    {
        std::list<std::shared_ptr<google::protobuf::Message>> msgs_out;
        try
        {
            while (!bytes1.empty())
                msgs_out.push_back(
                    codec.decode<std::shared_ptr<google::protobuf::Message>>(&bytes1));
        }
        catch (dccl::Exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        std::list<const google::protobuf::Message*>::const_iterator in_it = msgs.begin();

        assert(msgs.size() == msgs_out.size());

        for (const auto& it : msgs_out)
        {
            static int i = 0;
            std::cout << "... got Message " << ++i << " out:\n" << it->DebugString() << std::endl;
            assert((*in_it)->SerializeAsString() == it->SerializeAsString());
            ++in_it;
        }
    }

    std::cout << "all tests passed" << std::endl;
}
