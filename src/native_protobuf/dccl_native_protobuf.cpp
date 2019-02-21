// Copyright 2009-2017 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
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

#include "dccl_native_protobuf.h"
#include "dccl/codec.h"

extern "C"
{
    void dccl3_load(dccl::Codec* dccl)
    {
        using namespace dccl;
        using namespace dccl::native_protobuf;
        using google::protobuf::FieldDescriptor;

        const char* native_pb_group = "dccl.native_protobuf";
        
        FieldCodecManager::add<v3::DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(native_pb_group);
        FieldCodecManager::add<PrimitiveTypeFieldCodec<dccl::uint32>, FieldDescriptor::TYPE_UINT32>(native_pb_group);

    }
    
    void dccl3_unload(dccl::Codec* dccl)
    {
        using namespace dccl;
        using namespace dccl::native_protobuf;
        using google::protobuf::FieldDescriptor;

        const char* native_pb_group = "dccl.native_protobuf";

        FieldCodecManager::remove<PrimitiveTypeFieldCodec<dccl::uint32>, FieldDescriptor::TYPE_UINT32>(native_pb_group);
        FieldCodecManager::remove<v3::DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(native_pb_group);
                    
    }
}
