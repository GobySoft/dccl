// Copyright 2011-2017:
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
// tests fixed id header

#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    {
        ShortIDMsg short_id_msg;
        codec.load(short_id_msg.GetDescriptor());
        codec.info(short_id_msg.GetDescriptor(), &dccl::dlog);

        std::string encoded;
        assert(codec.size(short_id_msg) == 1);
        codec.encode(&encoded, short_id_msg);
        assert(codec.id(encoded) == 2);
        codec.decode(encoded, &short_id_msg);
    }

    {
        LongIDMsg long_id_msg;
        std::string encoded;
        codec.load(long_id_msg.GetDescriptor());
        codec.info(long_id_msg.GetDescriptor(), &dccl::dlog);
        assert(codec.size(long_id_msg) == 2);
        codec.encode(&encoded, long_id_msg);
        assert(codec.id(encoded) == 10000);
        codec.decode(encoded, &long_id_msg);
    }

    {
        ShortIDEdgeMsg short_id_edge_msg;
        std::string encoded;
        codec.load(short_id_edge_msg.GetDescriptor());
        codec.info(short_id_edge_msg.GetDescriptor(), &dccl::dlog);
        assert(codec.size(short_id_edge_msg) == 1);
        codec.encode(&encoded, short_id_edge_msg);
        assert(codec.id(encoded) == 127);
        codec.decode(encoded, &short_id_edge_msg);
    }

    {
        LongIDEdgeMsg long_id_edge_msg;
        std::string encoded;
        codec.load(long_id_edge_msg.GetDescriptor());
        codec.info(long_id_edge_msg.GetDescriptor(), &dccl::dlog);
        codec.encode(&encoded, long_id_edge_msg);
        assert(codec.id(encoded) == 128);
        codec.decode(encoded, &long_id_edge_msg);
        assert(codec.size(long_id_edge_msg) == 2);
    }

    {
        TooLongIDMsg too_long_id_msg;
        // should fail validation
        try
        {
            codec.load(too_long_id_msg.GetDescriptor());
            assert(false);
        }
        catch (dccl::Exception& e)
        {
        }
    }

    {
        ShortIDMsgWithData short_id_msg_with_data;
        std::string encoded;
        codec.load(short_id_msg_with_data.GetDescriptor());
        codec.info(short_id_msg_with_data.GetDescriptor(), &dccl::dlog);

        short_id_msg_with_data.set_in_head(42);
        short_id_msg_with_data.set_in_body(37);
        codec.encode(&encoded, short_id_msg_with_data);
        assert(codec.id(encoded) == 3);
        codec.decode(encoded, &short_id_msg_with_data);
    }

    std::cout << "all tests passed" << std::endl;
}
