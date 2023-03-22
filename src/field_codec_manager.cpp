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

dccl::FieldCodecManagerLocal dccl::FieldCodecManager::meta_manager_(
    false /* must not enroll or we will copy uninitialized data */);
std::set<dccl::FieldCodecManagerLocal*> dccl::FieldCodecManager::managers_({&meta_manager_});

dccl::FieldCodecManagerLocal::FieldCodecManagerLocal(bool enroll) : enroll_(enroll)
{
    std::cout << "Starting FieldCodecManagerLocal: " << this << " enroll: " << std::boolalpha
              << enroll << std::endl;

    if (enroll_)
        FieldCodecManager::enroll(this);
}

dccl::FieldCodecManagerLocal::~FieldCodecManagerLocal()
{
    if (enroll_)
        FieldCodecManager::unenroll(this);
}

boost::shared_ptr<dccl::FieldCodecBase>
dccl::FieldCodecManagerLocal::__find(google::protobuf::FieldDescriptor::Type type,
                                     const std::string& codec_name,
                                     const std::string& type_name /* = "" */) const
{
    auto it = codecs_.find(type);
    if (it != codecs_.end())
    {
        auto inside_it = it->second.end();
        // try specific type codec
        inside_it = it->second.find(__mangle_name(codec_name, type_name));
        if (inside_it != it->second.end())
            return inside_it->second;

        // try general
        inside_it = it->second.find(codec_name);
        if (inside_it != it->second.end())
            return inside_it->second;
    }

    throw(Exception("No codec by the name `" + codec_name +
                    "` found for type: " + type_helper().find(type)->as_str()));
}
