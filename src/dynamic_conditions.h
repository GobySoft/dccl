// Copyright 2022:
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
#ifndef DCCLDYNAMICCONDITIONALS20220214H
#define DCCLDYNAMICCONDITIONALS20220214H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "dccl/option_extensions.pb.h"
#include "def.h"

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

    void set_repeated_index(int index) { index_ = index; }

    void regenerate(const google::protobuf::Message* this_msg,
                    const google::protobuf::Message* root_msg, int repeated_index = -1);

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

    bool is_initialized() { return root_msg_ && this_msg_ && field_desc_; }
    const google::protobuf::FieldDescriptor* field_desc_{nullptr};
    const google::protobuf::Message* this_msg_{nullptr};
    const google::protobuf::Message* root_msg_{nullptr};
    int index_{0};

#if DCCL_HAS_LUA
    sol::state* lua_{nullptr};
#endif
};

} // namespace dccl

#endif
