// Copyright 2011-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Davide Fenucci <davfen@noc.ac.uk>
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
// tests all protobuf types with _default codecs, repeat and non repeat

#include <fstream>

#include "../../native_protobuf/dccl_native_protobuf.h"
#include <google/protobuf/descriptor.pb.h>

#include "../../binary.h"
#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

dccl::Codec codec;

template <typename Msg1, typename Msg2>

std::pair<std::size_t, std::size_t> compute_hashes()
{
    std::size_t hash1 = codec.load<Msg1>();
    codec.unload<Msg1>();
    std::cout << Msg1::descriptor()->full_name() << ": " << dccl::hash_as_string(hash1)
              << std::endl;
    std::size_t hash2 = codec.load<Msg2>();
    codec.unload<Msg2>();
    std::cout << Msg2::descriptor()->full_name() << ": " << dccl::hash_as_string(hash2)
              << std::endl;
    return std::make_pair(hash1, hash2);
}

template <typename Msg1, typename Msg2> void expect_same()
{
    auto hashes = compute_hashes<Msg1, Msg2>();
    assert(hashes.first == hashes.second);
}

template <typename Msg1, typename Msg2> void expect_different()
{
    auto hashes = compute_hashes<Msg1, Msg2>();
    assert(hashes.first != hashes.second);
}

int main(int /*argc*/, char* /*argv*/[])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    expect_same<TestMsg, TestMsgNoHashableChanges>();
    expect_different<TestMsg, TestMsgNewID>();
    expect_different<TestMsg, TestMsgNewEnum>();
    expect_different<TestMsg, TestMsgNewBounds>();
    expect_different<TestMsgV2, TestMsgV3>();
    expect_different<TestMsgV3, TestMsgV4>();
}
