// Copyright 2026:
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
// tests encryption/decryption via Codec::set_crypto_passphrase

#include <cassert>
#include <iostream>
#include <string>

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"

using namespace dccl::test;

namespace
{
CryptoMsg make_msg()
{
    CryptoMsg msg;
    msg.set_source(5);
    msg.set_x(1234);
    msg.set_y(-5678);
    msg.set_message("hello");
    msg.set_hash(0); // dummy value - overwritten by the dccl.hash codec
    return msg;
}

bool same_contents(const CryptoMsg& a, const CryptoMsg& b)
{
    return a.source() == b.source() && a.x() == b.x() && a.y() == b.y() &&
           a.message() == b.message();
}

std::string encode(dccl::Codec& codec, const google::protobuf::Message& msg)
{
    std::string bytes;
    codec.encode(&bytes, msg);
    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "encoded (hex): " << dccl::hex_encode(bytes)
                                                    << std::endl;
    return bytes;
}
} // namespace

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

#if !DCCL_HAS_CRYPTOPP
    std::cerr << "DCCL compiled without Crypto++: skipping encryption test." << std::endl;
    return 0;
#else
    const CryptoMsg msg_in = make_msg();

    dccl::Codec plain_codec;
    plain_codec.load<CryptoMsg>();
    const std::string plain_bytes = encode(plain_codec, msg_in);

    //
    // round trip with the same passphrase
    //
    {
        dccl::Codec codec;
        codec.set_crypto_passphrase("my_passphrase");
        codec.load<CryptoMsg>();

        const std::string bytes = encode(codec, msg_in);
        assert(bytes.size() == plain_bytes.size());
        assert(bytes != plain_bytes);

        // only the body is encrypted; the head is the nonce and stays in the clear
        std::string head;
        plain_codec.encode(&head, msg_in, /* header_only = */ true);
        const std::size_t head_bytes = head.size();
        assert(bytes.substr(0, head_bytes) == plain_bytes.substr(0, head_bytes));
        assert(bytes.substr(head_bytes) != plain_bytes.substr(head_bytes));

        CryptoMsg msg_out;
        codec.decode(bytes, &msg_out);
        assert(same_contents(msg_in, msg_out));

        // the DCCL id remains readable without the passphrase
        assert(plain_codec.id(bytes) == plain_codec.id<CryptoMsg>());
    }

    //
    // a different passphrase must be rejected, not silently decoded as garbage
    //
    {
        dccl::Codec enc_codec, dec_codec;
        enc_codec.set_crypto_passphrase("my_passphrase");
        dec_codec.set_crypto_passphrase("wrong_passphrase");
        enc_codec.load<CryptoMsg>();
        dec_codec.load<CryptoMsg>();

        const std::string bytes = encode(enc_codec, msg_in);

        bool rejected = false;
        try
        {
            CryptoMsg msg_out;
            dec_codec.decode(bytes, &msg_out);
        }
        catch (const std::exception& e)
        {
            rejected = true;
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "expected decode failure: " << e.what() << std::endl;
        }
        assert(rejected);
    }

    //
    // the dccl.hash field also catches corruption of an otherwise valid message
    //
    {
        dccl::Codec codec;
        codec.set_crypto_passphrase("my_passphrase");
        codec.load<CryptoMsg>();

        std::string bytes = encode(codec, msg_in);
        bytes[bytes.size() - 1] ^= 0x01; // one bit of ciphertext, so one bit of body

        bool rejected = false;
        try
        {
            CryptoMsg msg_out;
            codec.decode(bytes, &msg_out);
        }
        catch (const std::exception& e)
        {
            rejected = true;
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "expected decode failure: " << e.what() << std::endl;
        }
        assert(rejected);
    }

    //
    // ids listed in do_not_encrypt_ids are left in the clear
    //
    {
        dccl::Codec codec;
        codec.set_crypto_passphrase("my_passphrase", std::set<dccl::int32>{21});
        codec.load<CryptoMsg>();
        codec.load<PlaintextMsg>();

        PlaintextMsg plain_in;
        plain_in.set_source(5);
        plain_in.set_x(1234);
        plain_in.set_y(-5678);
        plain_in.set_message("hello");
        plain_in.set_hash(0);

        plain_codec.load<PlaintextMsg>();
        assert(encode(codec, plain_in) == encode(plain_codec, plain_in));
        assert(encode(codec, msg_in) != plain_bytes);
    }

    std::cout << "all tests passed" << std::endl;
    return 0;
#endif
}
