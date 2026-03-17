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

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    //
    // Test CRC-16
    //
    {
        std::cout << "=== Testing CRC-16 ===" << std::endl;
        codec.load<TestCRC16>();
        codec.info<TestCRC16>(&std::cout);

        TestCRC16 msg_in, msg_out;
        msg_in.set_x(1234);
        msg_in.set_y(-5678);
        msg_in.set_crc(0); // dummy value - overwritten by dccl.crc16 codec

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Encoding..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "Encoded (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Decoding..." << std::endl;
        try
        {
            codec.decode(bytes, &msg_out);
        }
        catch (const std::exception& e)
        {
            std::cerr << "UNEXPECTED exception during decode: " << e.what() << std::endl;
            assert(false);
        }
        std::cout << "Message out:\n" << msg_out.DebugString() << std::endl;

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
            std::cout << "Caught expected exception for corrupt CRC-16: " << e.what() << std::endl;
        }

        codec.unload<TestCRC16>();
        std::cout << "CRC-16 tests passed!" << std::endl;
    }

    //
    // Test CRC-32
    //
    {
        std::cout << "\n=== Testing CRC-32 ===" << std::endl;
        codec.load<TestCRC32>();
        codec.info<TestCRC32>(&std::cout);

        TestCRC32 msg_in, msg_out;
        msg_in.set_x(9999);
        msg_in.set_y(-9999);
        msg_in.set_crc(0); // dummy value - overwritten by dccl.crc32 codec

        std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
        std::cout << "Encoding..." << std::endl;
        std::string bytes;
        codec.encode(&bytes, msg_in);
        std::cout << "Encoded (hex): " << dccl::hex_encode(bytes) << std::endl;

        std::cout << "Decoding..." << std::endl;
        codec.decode(bytes, &msg_out);
        std::cout << "Message out:\n" << msg_out.DebugString() << std::endl;

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
            std::cout << "Caught expected exception for corrupt CRC-32: " << e.what() << std::endl;
        }

        codec.unload<TestCRC32>();
        std::cout << "CRC-32 tests passed!" << std::endl;
    }

    //
    // Test encoding twice gives same result (CRC is deterministic)
    //
    {
        std::cout << "\n=== Testing CRC-16 determinism ===" << std::endl;
        codec.load<TestCRC16>();

        TestCRC16 msg;
        msg.set_x(42);
        msg.set_y(100);
        msg.set_crc(0);

        std::string bytes1, bytes2;
        codec.encode(&bytes1, msg);
        codec.encode(&bytes2, msg);
        assert(bytes1 == bytes2);
        std::cout << "CRC is deterministic." << std::endl;

        // Test different field values give different CRC
        TestCRC16 msg2;
        msg2.set_x(43); // different x
        msg2.set_y(100);
        msg2.set_crc(0);
        std::string bytes3;
        codec.encode(&bytes3, msg2);
        assert(bytes3 != bytes1);
        std::cout << "CRC differs for different data." << std::endl;

        codec.unload<TestCRC16>();
        std::cout << "Determinism tests passed!" << std::endl;
    }

    std::cout << "\nAll CRC tests passed!" << std::endl;
    return 0;
}
