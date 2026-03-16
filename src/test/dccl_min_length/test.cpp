// Copyright 2024:
//   GobySoft, LLC (2013-)
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
// tests min_length for bytes fields (VarBytesCodec) and v4 string fields

#include "dccl/binary.h"
#include "dccl/codec.h"
#include "test.pb.h"

using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;
    codec.load<MinLengthMsg>();
    codec.info<MinLengthMsg>(&dccl::dlog);

    // --- Basic encode/decode round-trip ---
    {
        MinLengthMsg msg_in;
        // req_bytes: min_length=4, max_length=10 — provide exactly min_length bytes
        msg_in.set_req_bytes(dccl::hex_decode("aabbccdd"));
        // opt_bytes: min_length=2, max_length=8 — provide more than min_length
        msg_in.set_opt_bytes(dccl::hex_decode("112233445566"));
        // str_field: min_length=3, max_length=10
        msg_in.set_str_field("hello");

        std::string encoded;
        codec.encode(&encoded, msg_in);

        MinLengthMsg msg_out;
        codec.decode(encoded, &msg_out);

        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
        std::cout << "Round-trip with fields at/above min_length: passed" << std::endl;
    }

    // --- Verify bit savings: a field with min_length reduces prefix size ---
    // req_bytes: min_length=4, max_length=10
    //   prefix_size = ceil_log2(10 - 4 + 1) = ceil_log2(7) = 3 bits
    // Without min_length: prefix_size = ceil_log2(10 + 1) = ceil_log2(11) = 4 bits
    // So we save 1 bit on the required field.
    {
        // DCCL ID = 8 bits
        // max_size for req_bytes (required, prefix_size=3): 3 + 10*8 = 83 bits
        // max_size for opt_bytes (optional, presence=1, prefix_size=ceil_log2(8-2+1)=3): 1+3+8*8=68 bits
        // max_size for str_field (optional, presence=1, prefix_size=ceil_log2(10-3+1)=3): 1+3+10*8=84 bits
        // total max_size = 8 + 83 + 68 + 84 = 235 bits = 31 bytes (rounded up)
        assert(codec.max_size<MinLengthMsg>() == 31);
        std::cout << "max_size check: passed (31 bytes)" << std::endl;
    }

    // --- Length less than min_length ---
    {
        MinLengthMsg msg_in;
        // req_bytes: min_length=4, max_length=10 — provide less than min_length
        msg_in.set_req_bytes(dccl::hex_decode("aabbcc"));

        std::string encoded;
        codec.encode(&encoded, msg_in);

        MinLengthMsg msg_out;
        codec.decode(encoded, &msg_out);

        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
        std::cout << "Round-trip with fields below min_length: passed" << std::endl;
    }
    

    // --- Optional bytes absent ---
    {
        MinLengthMsg msg_in;
        msg_in.set_req_bytes(dccl::hex_decode("aabbccdd"));
        // opt_bytes not set
        msg_in.set_str_field("abc");

        std::string encoded;
        codec.encode(&encoded, msg_in);

        MinLengthMsg msg_out;
        codec.decode(encoded, &msg_out);

        assert(!msg_out.has_opt_bytes());
        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
        std::cout << "Optional bytes absent: passed" << std::endl;
    }

    // --- Fixed-size bytes (min_length == max_length) ---
    {
        codec.load<FixedLengthMsg>();
        codec.info<FixedLengthMsg>(&dccl::dlog);

        FixedLengthMsg msg_in;
        msg_in.set_fixed_bytes(dccl::hex_decode("0102030405"));

        std::string encoded;
        codec.encode(&encoded, msg_in);

        FixedLengthMsg msg_out;
        codec.decode(encoded, &msg_out);

        assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());

        // prefix_size = ceil_log2(5-5+1) = ceil_log2(1) = 0 bits
        // max_size = 0 + 5*8 = 40 bits = 5 bytes
        assert(codec.max_size<FixedLengthMsg>() == 5);
        std::cout << "Fixed-size bytes (min_length == max_length): passed" << std::endl;
    }

    // --- Invalid: min_length > max_length should throw on load ---
    {
        try
        {
            codec.load<InvalidMinLengthMsg>();
            assert(false && "Expected exception for min_length > max_length");
        }
        catch (const dccl::Exception& e)
        {
            std::cout << "Invalid min_length > max_length correctly rejected: " << e.what()
                      << std::endl;
        }
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
