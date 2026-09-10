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

// Tests the Codec public API around the encode/decode happy path that the
// other suites already cover: the raw char-buffer overload, load/unload
// bookkeeping, size queries, the info() reports, and the load- and
// encode-time error paths.

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "dccl/codec.h"

#include "test.pb.h"

namespace
{
int checks = 0;

void check(bool condition, const std::string& what)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAILED: " << what << std::endl;
        exit(1);
    }
}

// Runs op and reports whether it threw the expected exception type.
template <typename Exception, typename Callable>
void check_throws(Callable op, const std::string& what)
{
    ++checks;
    try
    {
        op();
    }
    catch (const Exception& e)
    {
        std::cout << "ok (threw as expected): " << what << ": " << e.what() << std::endl;
        return;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAILED: " << what << " threw the wrong type: " << e.what() << std::endl;
        exit(1);
    }
    std::cerr << "FAILED: " << what << " did not throw" << std::endl;
    exit(1);
}

void test_raw_buffer_encode()
{
    dccl::Codec codec;
    codec.load<dccl::test::SimpleMsg>();

    dccl::test::SimpleMsg msg;
    msg.set_a(42);
    msg.set_b(7);

    std::string as_string;
    codec.encode(&as_string, msg);

    char buffer[256];
    size_t written = codec.encode(buffer, sizeof(buffer), msg);

    check(written == as_string.size(), "char* and std::string overloads agree on size");
    check(std::string(buffer, written) == as_string,
          "char* and std::string overloads agree on bytes");
    check(written == codec.size(msg), "size() matches the encoded length");

    dccl::test::SimpleMsg decoded;
    codec.decode(std::string(buffer, written), &decoded);
    check(decoded.a() == 42 && decoded.b() == 7, "raw buffer round trips");

    // header_only stops after the head, so it must be shorter
    char head_buffer[256];
    size_t head_written = codec.encode(head_buffer, sizeof(head_buffer), msg, true);
    check(head_written > 0, "header-only encode writes something");
    check(head_written < written, "header-only encode is shorter than a full encode");

    // a buffer too small for even the head, and one too small for head+body
    check_throws<std::length_error>([&]() { codec.encode(buffer, 0, msg); },
                                    "encode into a zero-length buffer");
    check_throws<std::length_error>([&]() { codec.encode(buffer, head_written, msg); },
                                    "encode into a buffer that only fits the head");
}

void test_load_and_unload()
{
    dccl::Codec codec;

    std::size_t hash = codec.load<dccl::test::SimpleMsg>();
    check(hash != 0, "load returns a hash");
    check(codec.load<dccl::test::SimpleMsg>() == hash, "loading twice yields the same hash");

    codec.load<dccl::test::OtherMsg>();
    check(codec.id<dccl::test::SimpleMsg>() == 220, "SimpleMsg keeps its declared id");
    check(codec.id<dccl::test::OtherMsg>() == 221, "OtherMsg keeps its declared id");

    // unload by descriptor, then by id
    codec.unload<dccl::test::SimpleMsg>();
    dccl::test::SimpleMsg msg;
    msg.set_a(1);
    check_throws<dccl::Exception>(
        [&]()
        {
            std::string s;
            codec.encode(&s, msg);
        },
        "encoding an unloaded message");

    codec.unload(221);
    dccl::test::OtherMsg other;
    other.set_c(1);
    check_throws<dccl::Exception>(
        [&]()
        {
            std::string s;
            codec.encode(&s, other);
        },
        "encoding a message unloaded by id");

    // unloading something that was never loaded is a no-op, not an error
    codec.unload(dccl::test::SimpleMsg::descriptor());
    codec.unload(9999);
    check(true, "unloading an unloaded message is a no-op");

    codec.load<dccl::test::SimpleMsg>();
    codec.load<dccl::test::OtherMsg>();
    codec.unload_all();
    check_throws<dccl::Exception>(
        [&]()
        {
            std::string s;
            codec.encode(&s, msg);
        },
        "encoding after unload_all");
}

void test_load_errors()
{
    dccl::Codec codec;

    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::NoMaxBytesMsg>(); },
                                  "loading a message without max_bytes");
    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::NoCodecVersionMsg>(); },
                                  "loading a message without codec_version");
    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::TooBigMsg>(); },
                                  "loading a message that exceeds its own max_bytes");

    // a second message claiming an id already in use must be rejected
    codec.load<dccl::test::SimpleMsg>();
    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::DuplicateIdMsg>(); },
                                  "loading a message with a duplicate DCCL id");

    // codec versions 2 and 3 predate 'oneof' support
    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::OneofV2Msg>(); },
                                  "loading a v2 message containing a oneof");
    check_throws<dccl::Exception>([&]() { codec.load<dccl::test::OneofV3Msg>(); },
                                  "loading a v3 message containing a oneof");
}

void test_encode_errors()
{
    dccl::Codec codec;
    codec.load<dccl::test::SimpleMsg>();

    // 'a' is required and unset
    dccl::test::SimpleMsg incomplete;
    check_throws<dccl::Exception>(
        [&]()
        {
            std::string s;
            codec.encode(&s, incomplete);
        },
        "encoding a message with an unset required field");

    // the report of missing fields walks into sub-messages and repeated
    // sub-messages
    codec.load<dccl::test::NestedMsg>();
    dccl::test::NestedMsg nested;
    nested.mutable_single(); // present but its own required field is unset
    nested.add_several();
    nested.add_several();

    bool named_the_field = false;
    try
    {
        std::string s;
        codec.encode(&s, nested);
    }
    catch (const dccl::Exception& e)
    {
        named_the_field = std::string(e.what()).find("inner_required") != std::string::npos;
    }
    check(named_the_field, "the error names the missing field inside a sub-message");

    // out-of-bounds values are clamped by default, and throw under strict
    dccl::test::SimpleMsg out_of_range;
    out_of_range.set_a(999999);
    std::string clamped;
    codec.encode(&clamped, out_of_range);
    check(!clamped.empty(), "an out-of-range field encodes when not strict");

    codec.set_strict(true);
    check_throws<dccl::OutOfRangeException>(
        [&]()
        {
            std::string s;
            codec.encode(&s, out_of_range);
        },
        "an out-of-range field under strict mode");
    codec.set_strict(false);
}

void test_size_queries()
{
    dccl::Codec codec;
    codec.load<dccl::test::SimpleMsg>();

    unsigned max = codec.max_size<dccl::test::SimpleMsg>();
    unsigned min = codec.min_size<dccl::test::SimpleMsg>();

    check(min <= max, "min_size does not exceed max_size");
    check(max <= 32, "max_size respects the declared max_bytes");

    dccl::test::SimpleMsg msg;
    msg.set_a(1);
    msg.set_b(2);
    unsigned actual = codec.size(msg);
    check(actual >= min && actual <= max, "an actual encoding falls between min and max size");

    // the descriptor overloads agree with the template ones
    check(codec.max_size(dccl::test::SimpleMsg::descriptor()) == max,
          "max_size descriptor overload agrees");
    check(codec.min_size(dccl::test::SimpleMsg::descriptor()) == min,
          "min_size descriptor overload agrees");
}

void test_info_output()
{
    dccl::Codec codec;
    codec.load<dccl::test::SimpleMsg>();
    codec.load<dccl::test::OtherMsg>();

    std::stringstream single;
    codec.info<dccl::test::SimpleMsg>(&single);
    check(single.str().find("SimpleMsg") != std::string::npos, "info() names the message");
    check(single.str().find("a") != std::string::npos, "info() lists fields");

    std::stringstream all;
    codec.info_all(&all);
    check(all.str().find("SimpleMsg") != std::string::npos, "info_all() covers SimpleMsg");
    check(all.str().find("OtherMsg") != std::string::npos, "info_all() covers OtherMsg");
    check(all.str().find("2 messages loaded") != std::string::npos,
          "info_all() reports how many messages are loaded");

    // a console narrower than the title leaves no room for the guard bars, and
    // must produce output rather than throwing
    codec.set_console_width(1);
    std::stringstream narrow;
    codec.info_all(&narrow);
    check(!narrow.str().empty(), "info_all() copes with a console narrower than its title");

    codec.set_console_width(200);
    std::stringstream wide;
    codec.info_all(&wide);
    check(!wide.str().empty(), "info_all() copes with a wide console");
}

void test_library_loading()
{
    dccl::Codec codec;

    check_throws<dccl::Exception>([&]() { codec.load_library(static_cast<void*>(nullptr)); },
                                  "load_library with a null handle");
    check_throws<dccl::Exception>([&]() { codec.unload_library(nullptr); },
                                  "unload_library with a null handle");
    check_throws<dccl::Exception>([&]() { codec.load_library("/nonexistent/libnope.so"); },
                                  "load_library with a path that does not resolve");
}

void test_id_codec()
{
    dccl::Codec codec;
    check(codec.get_id_codec() == dccl::Codec::default_id_codec_name(),
          "a default-constructed Codec uses the default id codec");

    check_throws<dccl::Exception>([&]() { dccl::Codec bad("no.such.id.codec"); },
                                  "constructing with an unknown id codec");

    // setting a codec unloads everything, since ids may be reassigned
    codec.load<dccl::test::SimpleMsg>();
    check_throws<dccl::Exception>([&]() { codec.set_id_codec("no.such.id.codec"); },
                                  "setting an unknown id codec");
}

void test_id_from_bytes()
{
    dccl::Codec codec;
    codec.load<dccl::test::SimpleMsg>();

    dccl::test::SimpleMsg msg;
    msg.set_a(5);
    std::string bytes;
    codec.encode(&bytes, msg);

    check(codec.id(bytes) == 220, "the id can be read back off the wire");
    check(codec.id(bytes.begin(), bytes.end()) == 220, "the iterator id() overload agrees");
    check(codec.id(dccl::test::SimpleMsg::descriptor()) == 220,
          "the descriptor id() overload agrees");
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

    test_raw_buffer_encode();
    test_load_and_unload();
    test_load_errors();
    test_encode_errors();
    test_size_queries();
    test_info_output();
    test_library_loading();
    test_id_codec();
    test_id_from_bytes();

    std::cout << checks << " checks passed" << std::endl;
    return 0;
}
