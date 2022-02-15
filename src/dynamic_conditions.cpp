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

#include "dccl/dynamic_conditions.h"
#include "dccl/exception.h"

#if DCCL_HAS_LUA
#include "dccl/thirdparty/sol/sol.hpp"
// symbol in lua-protobuf/pb.c so we can load this using sol's require call
LUALIB_API int luaopen_pb(lua_State *L);
#define SOL_ALL_SAFETIES_ON 1
#define SOL_PRINT_ERRORS 1
#endif


void build_file_desc_set(const google::protobuf::FileDescriptor* file_desc,
                         google::protobuf::FileDescriptorSet& file_desc_set)
{
    for (int i = 0, n = file_desc->dependency_count(); i < n; ++i)
        build_file_desc_set(file_desc->dependency(i), file_desc_set);

    google::protobuf::FileDescriptorProto* file_desc_proto = file_desc_set.add_file();
    file_desc->CopyTo(file_desc_proto);
}

dccl::DynamicConditions::DynamicConditions() {}

dccl::DynamicConditions::~DynamicConditions()
{
#if DCCL_HAS_LUA
    if (lua_)
        delete lua_;
#endif
}

void dccl::DynamicConditions::set_message(const google::protobuf::Message* msg)
{
    msg_ = msg;
#if DCCL_HAS_LUA
    if (msg_)
    {
        if (!lua_)
        {
            lua_ = new sol::state;
            lua_->open_libraries();
            lua_->require("pb", luaopen_pb);
        }

        sol::load_result desc_load = lua_->load(R"(local desc = ...; return pb.load(desc) )");

        google::protobuf::FileDescriptorSet file_desc_set;
        build_file_desc_set(msg_->GetDescriptor()->file(), file_desc_set);

        std::tuple<bool, int> desc_load_result = desc_load(file_desc_set.SerializeAsString());
        assert(std::get<0>(desc_load_result));
        const auto& decode_script =
            "local encoded_msg, type = ...; pb.option('use_default_metatable'); this = "
            "pb.decode(type, encoded_msg); return this";

        sol::load_result decode_message = lua_->load(decode_script);
        if (!decode_message.valid())
        {
            sol::error err = decode_message;
            throw(Exception(std::string("Failed to load condition script into the Lua program: ") +
                            err.what()));
        }

        sol::table decoded_message =
            decode_message(msg_->SerializePartialAsString(), msg_->GetDescriptor()->full_name());
    }
#endif
}


const dccl::DCCLFieldOptions::Conditions& dccl::DynamicConditions::conditions()
{
    if (field_desc_)
        return field_desc_->options().GetExtension(dccl::field).dynamic_conditions();
    else
        throw(Exception("Null field_desc"));
}

bool dccl::DynamicConditions::required()
{
#if DCCL_HAS_LUA
    if (msg_ && field_desc_)
    {
        if (conditions().has_required_if())
        {
            auto condition_script = return_prefix(conditions().required_if());
            bool required = lua_->script(condition_script);
            return required;
        }
        else if (conditions().has_only_if())
        {
            auto condition_script = return_prefix(conditions().only_if());
            bool only = lua_->script(condition_script);
            return only;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

#else
    throw(Exception("DCCL built without Lua support: cannot use dynamic_conditions"));
#endif
}

bool dccl::DynamicConditions::omit()
{
#if DCCL_HAS_LUA
    if (msg_ && field_desc_)
    {
        if (conditions().has_omit_if())
        {
            auto condition_script = return_prefix(conditions().omit_if());
            bool omit = lua_->script(condition_script);
            return omit;
        }
        else if (conditions().has_only_if())
        {
            auto condition_script = return_prefix(conditions().only_if());
            bool only = lua_->script(condition_script);
            return !only;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
#else
    throw(Exception("DCCL built without Lua support: cannot use dynamic_conditions"));
#endif
}

double dccl::DynamicConditions::min()
{
#if DCCL_HAS_LUA
    if (msg_ && field_desc_)
    {
        auto condition_script = return_prefix(conditions().min());
        double v = lua_->script(condition_script);
        return v;
    }
    else
    {
        return -std::numeric_limits<double>::infinity();
    }
#else
    throw(Exception("DCCL built without Lua support: cannot use dynamic_conditions"));
#endif
}

double dccl::DynamicConditions::max()
{
#if DCCL_HAS_LUA
    if (msg_ && field_desc_)
    {
        auto condition_script = return_prefix(conditions().max());
        double v = lua_->script(condition_script);
        return v;
    }
    else
    {
        return std::numeric_limits<double>::infinity();
    }
#else
    throw(Exception("DCCL built without Lua support: cannot use dynamic_conditions"));
#endif
}
