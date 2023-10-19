// Copyright 2011-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
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
#include "field_codec_manager.h"

dccl::FieldCodecManagerLocal::FieldCodecManagerLocal()
    : deprecated_names_{{"_static", "dccl.static"}, {"_time", "dccl.time"}}
{
}

dccl::FieldCodecManagerLocal::~FieldCodecManagerLocal() = default;

std::shared_ptr<dccl::FieldCodecBase>
dccl::FieldCodecManagerLocal::__find(google::protobuf::FieldDescriptor::Type type,
                                     int codec_version, const std::string& codec_name,
                                     const std::string& type_name) const
{
    check_deprecated(codec_name);

    auto it = codecs_.find(type);

    std::vector<std::string> codec_names_to_try;
    if (it != codecs_.end())
    {
        auto inside_it = it->second.end();

        // try appending codec_version first
        if (!std::isdigit(codec_name.back()))
            codec_names_to_try.push_back(codec_name + std::to_string(codec_version));

        codec_names_to_try.push_back(codec_name);

        for (const std::string& c_name : codec_names_to_try)
        {
            // try specific type codec
            inside_it = it->second.find(__mangle_name(c_name, type_name));
            if (inside_it != it->second.end())
                return inside_it->second;

            // try general
            inside_it = it->second.find(c_name);
            if (inside_it != it->second.end())
                return inside_it->second;
        }
    }

    std::stringstream err_ss;
    err_ss << "No codec by the name `" << codec_name
           << "` found for type: " << type_helper().find(type)->as_str() << " (tried names:";
    for (const std::string& c_name : codec_names_to_try) err_ss << " `" << c_name << "`";
    err_ss << ")";
    throw(Exception(err_ss.str()));
}

void dccl::FieldCodecManagerLocal::check_deprecated(const std::string& codec_name) const
{
    auto it = deprecated_names_.find(codec_name);
    if (it != deprecated_names_.end())
    {
        dlog.is(dccl::logger::DEBUG1) && dlog << "Codec name \"" << it->first
                                              << "\" is deprecated: use \"" << it->second
                                              << "\" instead " << std::endl;
    }
}
