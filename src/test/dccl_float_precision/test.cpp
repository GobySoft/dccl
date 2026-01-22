// Copyright 2011-2026:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Cadmus To <cadmus.to+dev@gmail.com>
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

// Tests encoding and decoding of different float precisions on DefaultNumericFieldCodec

#include "../../codec.h"
#include "test.pb.h"
using namespace dccl::test;

int main(int /*argc*/, char* /*argv*/ [])
{
    dccl::dlog.connect(dccl::logger::ALL, &std::cerr);

    dccl::Codec codec;

    // Simple testing
    codec.load<FloatMsg>();
    codec.info<FloatMsg>(&dccl::dlog);

    {
        const auto *field_ptr = FloatMsg::GetDescriptor()->FindFieldByName("f");
        assert(field_ptr);
        const auto &field = *field_ptr;

        const auto &opts = field.options();
        const auto &dccl_ext = opts.GetExtension(dccl::field);

        auto res = std::numeric_limits<double>::quiet_NaN();
        if (dccl_ext.has_precision()) {
            res = std::pow(10.0, -dccl_ext.precision());
        } else {
            res = dccl_ext.resolution();
        }

        const auto tol = res/2;
        std::cout << "Tolerance for DCCL float = " << res << " / 2 = " << tol << std::endl;

        const auto test_cases = std::array<float, 3>{0.000001f, 0.010001f, 1.000001f};
        for (const auto test_case : test_cases) {
            auto msg_in = FloatMsg{};
            msg_in.set_f(test_case);

            auto encoded = std::string{};
            codec.encode(&encoded, msg_in);

            auto msg_out = FloatMsg{};
            codec.decode(encoded, &msg_out);

            std::cout << "Checking if encoded value " << msg_out.f() << " is close enough to " << test_case << "..." << std::endl;
            assert(std::abs(test_case - msg_out.f()) < tol);
        }
    }

    std::cout << "all tests passed" << std::endl;
}
