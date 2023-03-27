// Copyright 2019:
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
// tests usage of a custom DCCL ID codec

#include "dccl/codec.h"
#include "dccl/field_codec_id.h"
#include "test.pb.h"
using namespace dccl::test;


namespace dccl
{
namespace test
{
// If "user_id" is set via UserCustomIdRAII, use that, fall back to using the
// default ID Codec
class UserCustomIdCodec : public DefaultIdentifierCodec
{
  private:
    dccl::Bitset encode(const dccl::uint32& wire_value) override
    {
        return user_id_set ? encode() : DefaultIdentifierCodec::encode(wire_value);
    }

    dccl::Bitset encode() override
    {
        return user_id_set ? dccl::Bitset() : DefaultIdentifierCodec::encode();
    }

    unsigned size() override { return user_id_set ? 0 : DefaultIdentifierCodec::size(); }

    unsigned size(const dccl::uint32& wire_value) override
    {
        return user_id_set ? 0 : DefaultIdentifierCodec::size(wire_value);
    }

    unsigned min_size() override { return user_id_set ? 0 : DefaultIdentifierCodec::min_size(); }

    unsigned max_size() override { return user_id_set ? 0 : DefaultIdentifierCodec::max_size(); }

    // pass the current ID back
    dccl::uint32 decode(dccl::Bitset* bits) override
    {
        return user_id_set ? user_id : DefaultIdentifierCodec::decode(bits);
    }

    void validate() override
    {
        if (!user_id_set)
            DefaultIdentifierCodec::validate();
    }

    friend struct UserCustomIdRAII;
    static dccl::uint32 user_id;
    static bool user_id_set;
};

// RAII-based tool for setting the current DCCL ID within the scope of this
// object's lifetime. Any actions (load, info, encode, decode, etc.) will
// use this DCCL ID for the duration of this lifetime.
struct UserCustomIdRAII
{
    UserCustomIdRAII(dccl::uint32 id)
    {
        UserCustomIdCodec::user_id = id;
        UserCustomIdCodec::user_id_set = true;
    }
    ~UserCustomIdRAII()
    {
        UserCustomIdCodec::user_id = 0;
        UserCustomIdCodec::user_id_set = false;
    }
};
} // namespace test
} // namespace dccl

bool dccl::test::UserCustomIdCodec::user_id_set = false;
dccl::uint32 dccl::test::UserCustomIdCodec::user_id = 0;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    {
        dccl::Codec codec("user_id_codec", dccl::test::UserCustomIdCodec());

        // load TestMessageA as DCCL ID 1
        {
            dccl::test::UserCustomIdRAII scoped_user_id(1);
            codec.load<TestMessageA>();
            codec.info<TestMessageA>(&dccl::dlog);
        }
        // load TestMessageA as DCCL ID 2
        {
            dccl::test::UserCustomIdRAII scoped_user_id(2);
            codec.load<TestMessageA>();
            codec.info<TestMessageA>(&dccl::dlog);
        }

        // load TestMessageB as DCCL ID 3
        {
            dccl::test::UserCustomIdRAII scoped_user_id(3);
            codec.load<TestMessageB>();
            codec.info<TestMessageB>(&dccl::dlog);
        }

        // load TestMessageA using default ID codec
        {
            codec.load<TestMessageA>();
            codec.info<TestMessageA>(&dccl::dlog);
        }

        // one byte as the UserCustomIdCodec doesn't use any bytes on the wire
        unsigned byte_size_user_specified = 1;
        TestMessageA a1;
        a1.set_a(10);

        TestMessageA a2;
        a2.set_a(20);

        TestMessageB b;
        b.set_b(30);

        // one byte for the data, one byte for the ID
        unsigned byte_size_default = 2;
        TestMessageA a_default;
        a_default.set_a(40);

        // encode/decode using user-specified DCCL ID
        {
            dccl::test::UserCustomIdRAII scoped_user_id(1);
            std::string bytes;

            std::cout << "A1: " << a1.ShortDebugString() << std::endl;
            assert(codec.size(a1) == byte_size_user_specified);
            codec.encode(&bytes, a1);
            assert(bytes.size() == byte_size_user_specified);
            TestMessageA a1_out;
            codec.decode(bytes, &a1_out);
            std::cout << "A1 decoded: " << a1_out.ShortDebugString() << std::endl;
            assert(a1_out.SerializeAsString() == a1.SerializeAsString());
        }

        {
            dccl::test::UserCustomIdRAII scoped_user_id(2);
            std::string bytes;
            std::cout << "A2: " << a2.ShortDebugString() << std::endl;
            assert(codec.size(a2) == byte_size_user_specified);
            codec.encode(&bytes, a2);
            assert(bytes.size() == byte_size_user_specified);
            TestMessageA a2_out;
            codec.decode(bytes, &a2_out);
            std::cout << "A2 decoded: " << a2_out.ShortDebugString() << std::endl;
            assert(a2_out.SerializeAsString() == a2.SerializeAsString());
        }

        {
            dccl::test::UserCustomIdRAII scoped_user_id(3);
            std::string bytes;
            std::cout << "B: " << b.ShortDebugString() << std::endl;
            assert(codec.size(b) == byte_size_user_specified);
            codec.encode(&bytes, b);
            assert(bytes.size() == byte_size_user_specified);
            TestMessageA b_out;
            codec.decode(bytes, &b_out);
            std::cout << "B decoded: " << b_out.ShortDebugString() << std::endl;
            assert(b_out.SerializeAsString() == b.SerializeAsString());
        }

        // encode/decode using default DCCL ID
        {
            std::string bytes;

            std::cout << "A Default: " << a_default.ShortDebugString() << std::endl;
            assert(codec.size(a_default) == byte_size_default);
            codec.encode(&bytes, a_default);
            assert(bytes.size() == byte_size_default);
            TestMessageA a_default_out;
            codec.decode(bytes, &a_default_out);
            std::cout << "A Default decoded: " << a_default_out.ShortDebugString() << std::endl;
            assert(a_default_out.SerializeAsString() == a_default.SerializeAsString());
        }
    }

    std::cout << "all tests passed" << std::endl;
}
