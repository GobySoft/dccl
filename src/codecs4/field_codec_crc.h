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
#ifndef DCCLV4FIELDCODECCRCN
#define DCCLV4FIELDCODECCRCN

#include <cstdint>
#include <sstream>
#include <string>

#include "field_codec_default.h"

namespace dccl
{
namespace v4
{

/// \brief Computes a CRC-16/IBM-3740 (CRC-16/CCITT-FALSE) checksum.
///
/// Polynomial: 0x1021, Initial value: 0xFFFF, Input/output reflection: false, Final XOR: 0x0000
/// \param data byte buffer to checksum
/// \param size number of bytes in \p data
/// \return 16-bit CRC value
inline uint16_t crc16(const char* data, size_t size)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= static_cast<uint16_t>(static_cast<uint8_t>(data[i])) << 8;
        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x8000)
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            else
                crc = static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

/// \brief Computes a CRC-32/ISO-HDLC checksum (standard Ethernet/zlib CRC-32).
///
/// Polynomial: 0x04C11DB7 (reflected: 0xEDB88320), Initial value: 0xFFFFFFFF,
/// Input/output reflection: true, Final XOR: 0xFFFFFFFF
/// \param data byte buffer to checksum
/// \param size number of bytes in \p data
/// \return 32-bit CRC value
inline uint32_t crc32(const char* data, size_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= static_cast<uint8_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/// \brief DCCL CRC field codec base class. Computes a CRC over all bits encoded before this field
/// (during encoding) and verifies it on decoding.
///
/// The CRC field must be the last field encoded in the message body to be effective.
/// The value in the proto message is ignored on encoding (the computed CRC is always used).
/// On decoding, an exception is thrown if the received CRC does not match the computed CRC.
///
/// \tparam CRCBits number of CRC bits (16 for CRC-16, 32 for CRC-32)
template <int CRCBits> class CRCCodecBase : public v4::DefaultNumericFieldCodec<uint32>
{
  public:
    CRCCodecBase() { this->set_force_use_required(); }

    double min() override { return 0; }
    double max() override { return (CRCBits == 32) ? static_cast<double>(0xFFFFFFFFu) : static_cast<double>(0xFFFFu); }

  private:
    Bitset encode() override
    {
        return this->v4::DefaultNumericFieldCodec<uint32>::encode(compute_crc_encode());
    }

    uint32 pre_encode(const uint32& /*field_value*/) override { return compute_crc_encode(); }

    uint32 post_decode(const uint32& wire_value) override
    {
        uint32 expected = compute_crc_decode();
        uint32 received = wire_value;
        if (expected != received)
        {
            std::stringstream ss;
            ss << "CRC mismatch. Expected: 0x" << std::hex << expected << ", received: 0x"
               << received
               << ". Message data may be corrupted.";
            throw(Exception(ss.str(), this->root_descriptor()));
        }
        return wire_value;
    }

    void validate() override
    {
        // min() and max() are hardcoded - skip the parent's has_min/has_max checks and
        // call validate_numeric_bounds() directly to verify type range constraints.
        this->validate_numeric_bounds();

        // the CRC is computed over the bits of the message it belongs to, and only the root
        // message's bits are recovered while decoding
        FieldCodecBase::require(this->this_descriptor() == this->root_descriptor(),
                                "CRC codecs are only supported in the root message, not in an "
                                "embedded message");
    }

    /// \brief Compute CRC over all bits accumulated so far (called during encoding).
    uint32 compute_crc_encode() const
    {
        const Bitset* bits = this->root_bitset();
        if (!bits)
            return 0;
        Bitset bits_copy = *bits;
        std::string byte_str = bits_copy.to_byte_string();
        return compute_crc(byte_str.data(), byte_str.size());
    }

    /// \brief Compute CRC over the bits decoded before this field (called during decoding).
    ///
    /// During decoding, root_bitset() points to decoded_bits_ which accumulates bits for all
    /// fields decoded before the CRC field. This is exactly the same set of bits as
    /// compute_crc_encode() covers, so both produce identical CRC values for valid messages.
    uint32 compute_crc_decode() const
    {
        const Bitset* bits = this->root_bitset();
        if (!bits)
            return 0;
        // decoded_bits_ already contains only the bits before the CRC field - no trimming needed.
        Bitset bits_copy = *bits;
        std::string byte_str = bits_copy.to_byte_string();
        return compute_crc(byte_str.data(), byte_str.size());
    }

    static uint32 compute_crc(const char* data, size_t size)
    {
        if (CRCBits == 16)
            return crc16(data, size);
        else
            return crc32(data, size);
    }
};

/// \brief DCCL CRC-16 codec (CRC-16/IBM-3740 / CCITT-FALSE).
///
/// Usage: `required uint32 crc = N [(dccl.field) = { codec: "dccl.crc16" }];`
/// The CRC field must be the last field in the message body.
class CRC16Codec : public CRCCodecBase<16>
{
};

/// \brief DCCL CRC-32 codec (CRC-32/ISO-HDLC).
///
/// Usage: `required uint32 crc = N [(dccl.field) = { codec: "dccl.crc32" }];`
/// The CRC field must be the last field in the message body.
class CRC32Codec : public CRCCodecBase<32>
{
};

} // namespace v4
} // namespace dccl

#endif
