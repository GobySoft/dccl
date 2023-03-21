// Copyright 2011-2018:
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
    msg_in.add_rep_bytes(dccl::hex_decode("0011223344556677"));
    msg_in.add_rep_bytes(dccl::hex_decode("8899aabbccddeeff"));

    std::string encoded;
    codec.encode(&encoded, msg_in);

    assert(encoded.size() == 36);

    BytesMsg msg_out;
    codec.decode(encoded, &msg_out);

    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

    std::cout << "all tests passed" << std::endl;
}
