// Copyright 2009-2022 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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

#ifndef DCCLDYNAMICCONDITIONALS20220214H
#define DCCLDYNAMICCONDITIONALS20220214H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "dccl/option_extensions.pb.h"

// clang-format off
#define DCCL_HAS_LUA @DCCL_HAS_LUA@
// clang-format on

namespace sol
{
class state;
}

namespace dccl
{
class DynamicConditions
{
  public:
    DynamicConditions();
    ~DynamicConditions();

    void set_field(const google::protobuf::FieldDescriptor* field_desc)
    {
        field_desc_ = field_desc;
    }

    void set_message(const google::protobuf::Message* msg);

    const dccl::DCCLFieldOptions::Conditions& conditions();

    bool has_required_if() { return conditions().has_required_if() || conditions().has_only_if(); }
    bool has_omit_if() { return conditions().has_omit_if() || conditions().has_only_if(); }
    bool has_only_if() { return conditions().has_only_if(); }
    bool has_min() { return conditions().has_min(); }
    bool has_max() { return conditions().has_max(); }

    bool required();
    bool omit();

    // required if true, omit if false
    bool only();
    double min();
    double max();

  private:
    std::string return_prefix(const std::string& script)
    {
        if (script.find("return") == std::string::npos)
            return "return " + script;
        else
            return script;
    }

    const google::protobuf::FieldDescriptor* field_desc_{nullptr};
    const google::protobuf::Message* msg_{nullptr};

#if DCCL_HAS_LUA
    sol::state* lua_;
#endif
};

} // namespace dccl

#endif
