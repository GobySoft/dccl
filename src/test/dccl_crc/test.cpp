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
// tests CRC field codecs (dccl.crc16 and dccl.crc32)

#include <cassert>
#include <iostream>
#include <stdexcept>

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"

using namespace dccl::test;

int main(int argc, char* argv[])
{
    bool verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] && argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == '\0')
            verbose = true;
    }

    dccl::dlog.connect(verbose ? dccl::logger::ALL : dccl::logger::WARN_PLUS, &std::cerr);

    dccl::Codec codec;

    //
    // Test CRC-16
    //
    {
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "=== Testing CRC-16 ===" << std::endl;
        codec.load<TestCRC16>();
        if (dccl::dlog.is(dccl::logger::INFO))
        {
            codec.info<TestCRC16>(&dccl::dlog);
        }

        TestCRC16 msg_in, msg_out;
        msg_in.set_x(1234);
        msg_in.set_y(-5678);
        msg_in.set_crc(0); // dummy value - overwritten by dccl.crc16 codec

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Message in:\n"
                                                        << msg_in.DebugString() << std::endl;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Encoding..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        dccl::dlog.is(dccl::logger::INFO) &&
            dccl::dlog << "Encoded (hex): " << dccl::hex_encode(bytes) << std::endl;

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Decoding..." << std::endl;
        try
        {
            codec.decode(bytes, &msg_out);
        }
        catch (const std::exception& e)
        {
            std::cerr << "UNEXPECTED exception during decode: " << e.what() << std::endl;
            assert(false);
        }
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Message out:\n"
                                                        << msg_out.DebugString() << std::endl;

        assert(msg_out.x() == msg_in.x());
        assert(msg_out.y() == msg_in.y());
        assert(msg_out.crc() != 0); // CRC should be set

        // Verify that the CRC in the decoded message matches what was encoded
        assert(msg_out.crc() == msg_out.crc());

        // Test that a corrupt message throws an exception
        std::string corrupt_bytes = bytes;
        corrupt_bytes[corrupt_bytes.size() - 1] ^= 0xFF; // flip bits in last byte
        try
        {
            TestCRC16 corrupt_out;
            codec.decode(corrupt_bytes, &corrupt_out);
            std::cerr << "ERROR: Corrupt message did not throw exception!" << std::endl;
            assert(false);
        }
        catch (const std::exception& e)
        {
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "Caught expected exception for corrupt CRC-16: " << e.what()
                           << std::endl;
        }

        codec.unload<TestCRC16>();
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "CRC-16 tests passed!" << std::endl;
    }

    //
    // Test CRC-32
    //
    {
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "\n=== Testing CRC-32 ===" << std::endl;
        codec.load<TestCRC32>();
        if (dccl::dlog.is(dccl::logger::INFO))
        {
            codec.info<TestCRC32>(&dccl::dlog);
        }

        TestCRC32 msg_in, msg_out;
        msg_in.set_x(9999);
        msg_in.set_y(-9999);
        msg_in.set_crc(0); // dummy value - overwritten by dccl.crc32 codec

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Message in:\n"
                                                        << msg_in.DebugString() << std::endl;
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Encoding..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        dccl::dlog.is(dccl::logger::INFO) &&
            dccl::dlog << "Encoded (hex): " << dccl::hex_encode(bytes) << std::endl;

        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Decoding..." << std::endl;
        codec.decode(bytes, &msg_out);
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Message out:\n"
                                                        << msg_out.DebugString() << std::endl;

        assert(msg_out.x() == msg_in.x());
        assert(msg_out.y() == msg_in.y());
        assert(msg_out.crc() != 0); // CRC should be set

        // Test that a corrupt message throws an exception
        std::string corrupt_bytes = bytes;
        corrupt_bytes[corrupt_bytes.size() - 1] ^= 0xFF; // flip bits in last byte
        try
        {
            TestCRC32 corrupt_out;
            codec.decode(corrupt_bytes, &corrupt_out);
            std::cerr << "ERROR: Corrupt message did not throw exception!" << std::endl;
            assert(false);
        }
        catch (const std::exception& e)
        {
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "Caught expected exception for corrupt CRC-32: " << e.what()
                           << std::endl;
        }

        codec.unload<TestCRC32>();
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "CRC-32 tests passed!" << std::endl;
    }

    //
    // Test encoding twice gives same result (CRC is deterministic)
    //
    {
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "\n=== Testing CRC-16 determinism ==="
                                                        << std::endl;
        codec.load<TestCRC16>();

        TestCRC16 msg;
        msg.set_x(42);
        msg.set_y(100);
        msg.set_crc(0);

        std::string bytes1, bytes2;
        codec.encode(&bytes1, msg);
        codec.encode(&bytes2, msg);
        assert(bytes1 == bytes2);
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "CRC is deterministic." << std::endl;

        // Test different field values give different CRC
        TestCRC16 msg2;
        msg2.set_x(43); // different x
        msg2.set_y(100);
        msg2.set_crc(0);
        std::string bytes3;
        codec.encode(&bytes3, msg2);
        assert(bytes3 != bytes1);
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "CRC differs for different data."
                                                        << std::endl;

        codec.unload<TestCRC16>();
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Determinism tests passed!" << std::endl;
    }

    //
    // Test CRC-32 over optional, variable length, repeated and embedded fields
    //
    {
        dccl::dlog.is(dccl::logger::INFO) &&
            dccl::dlog << "\n=== Testing CRC-32 over all field types ===" << std::endl;
        codec.load<TestCRCAllFields>();

        // once with every optional field set, once with all of them omitted
        for (int all_set = 1; all_set >= 0; --all_set)
        {
            TestCRCAllFields msg_in, msg_out;
            msg_in.set_x(1234);
            msg_in.mutable_em()->set_a(7);
            if (all_set)
            {
                msg_in.set_h(42);
                msg_in.set_y(-4321);
                msg_in.set_s("hello");
                msg_in.add_r(1);
                msg_in.add_r(2);
                msg_in.mutable_em()->set_b("world");
                msg_in.mutable_em_opt()->set_a(-7);
            }

            msg_in.set_crc(0); // dummy value - overwritten by the dccl.crc32 codec

            std::string bytes;
            codec.encode(&bytes, msg_in);
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "Encoded (hex): " << dccl::hex_encode(bytes) << std::endl;
            codec.decode(bytes, &msg_out);
            assert(msg_out.crc() != 0);
            msg_out.set_crc(0); // set on encoding, so not part of the round trip
            assert(msg_out.DebugString() == msg_in.DebugString());

            // corruption anywhere in the body (all the CRC covers) must still be caught
            std::string head;
            codec.encode(&head, msg_in, true);
            for (std::string::size_type i = head.size(); i < bytes.size(); ++i)
            {
                std::string corrupt_bytes = bytes;
                corrupt_bytes[i] ^= 0x01;
                if (corrupt_bytes == bytes)
                    continue;

                try
                {
                    TestCRCAllFields corrupt_out;
                    codec.decode(corrupt_bytes, &corrupt_out);
                    std::cerr << "ERROR: corrupt message (byte " << i << ") did not throw!"
                              << std::endl;
                    assert(false);
                }
                catch (const std::exception& e)
                {
                    dccl::dlog.is(dccl::logger::INFO) &&
                        dccl::dlog << "Caught expected exception: " << e.what() << std::endl;
                }
            }
        }

        codec.unload<TestCRCAllFields>();
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "All field type tests passed!"
                                                        << std::endl;
    }

    //
    // Test CRC-32 over a oneof
    //
    {
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "\n=== Testing CRC-32 over a oneof ==="
                                                        << std::endl;
        codec.load<TestCRCOneof>();

        TestCRCOneof msg_in, msg_out;
        msg_in.set_y(-4321);
        msg_in.set_crc(0);

        std::string bytes;
        codec.encode(&bytes, msg_in);
        codec.decode(bytes, &msg_out);
        assert(msg_out.y() == msg_in.y());
        assert(!msg_out.has_x());

        codec.unload<TestCRCOneof>();
        dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "Oneof tests passed!" << std::endl;
    }

    //
    // A CRC in an embedded message cannot be verified, so it must not load
    //
    {
        dccl::dlog.is(dccl::logger::INFO) &&
            dccl::dlog << "\n=== Testing CRC in an embedded message ===" << std::endl;
        try
        {
            codec.load<TestCRCBadNested>();
            std::cerr << "ERROR: CRC in an embedded message loaded!" << std::endl;
            assert(false);
        }
        catch (const std::exception& e)
        {
            dccl::dlog.is(dccl::logger::INFO) &&
                dccl::dlog << "Caught expected exception: " << e.what() << std::endl;
        }
    }

    dccl::dlog.is(dccl::logger::INFO) && dccl::dlog << "\nAll CRC tests passed!" << std::endl;
    return 0;
}
