// Copyright 2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
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

#include "codec.h"

#ifndef CODEC_VERSION
#error CODEC_VERSION must be set
#endif

#ifndef CODEC_VERSION_NAMESPACE
#error CODEC_VERSION_NAMESPACE must be set
#endif

#ifndef DCCL_DEFAULT_FIELD_CODEC_HEADER_H
#define DCCL_DEFAULT_FIELD_CODEC_HEADER_H

namespace dccl
{
//
// Helper functions to reduce copy/paste in set_default_codecs()
//
template <int version> struct DefaultFieldCodecAdder
{
    template <int dummy> static void add(FieldCodecManagerLocal& manager);
    template <int dummy, typename T, typename... Types>
    static void add(FieldCodecManagerLocal& manager);
};

template <int version> void add_time_field_codecs(FieldCodecManagerLocal& manager) {}
template <int version, typename T, typename... Types>
void add_time_field_codecs(FieldCodecManagerLocal& manager)
{
    std::string name = "dccl.time" + std::to_string(version);
    switch (version)
    {
        case 2: manager.add<v2::TimeCodec<T>>(name); break;
        case 3: manager.add<v3::TimeCodec<T>>(name); break;
        case 4: manager.add<v4::TimeCodec<T>>(name); break;
    }
    add_time_field_codecs<version, Types...>(manager);
}

template <int version> void add_static_field_codecs(FieldCodecManagerLocal& manager) {}
template <int version, typename T, typename... Types>
void add_static_field_codecs(FieldCodecManagerLocal& manager)
{
    std::string name = "dccl.static" + std::to_string(version);
    switch (version)
    {
        case 2: manager.add<v2::StaticCodec<T>>(name); break;
        case 3: manager.add<v3::StaticCodec<T>>(name); break;
        case 4: manager.add<v4::StaticCodec<T>>(name); break;
    }
    add_static_field_codecs<version, Types...>(manager);
}

template <int version> void add_presence_field_codecs(FieldCodecManagerLocal& manager)
{
    std::string name = "dccl.presence" + std::to_string(version);
    switch (version)
    {
        case 3: manager.add<v3::PresenceBitCodec<v3::DefaultEnumCodec>>(name); break;
        case 4: manager.add<v4::PresenceBitCodec<v4::DefaultEnumCodec>>(name); break;
    }
}
template <int version, typename T, typename... Types>
void add_presence_field_codecs(FieldCodecManagerLocal& manager)
{
    std::string name = "dccl.presence" + std::to_string(version);
    switch (version)
    {
        case 3: manager.add<v3::PresenceBitCodec<v3::DefaultNumericFieldCodec<T>>>(name); break;
        case 4: manager.add<v4::PresenceBitCodec<v4::DefaultNumericFieldCodec<T>>>(name); break;
    }
    add_presence_field_codecs<version, Types...>(manager);
}
} // namespace dccl
#endif

namespace dccl
{

template <>
template <int dummy>
void DefaultFieldCodecAdder<CODEC_VERSION>::add(FieldCodecManagerLocal& manager)
{
    using namespace CODEC_VERSION_NAMESPACE;
    using google::protobuf::FieldDescriptor;

    std::string name = dccl::Codec::default_codec_name(CODEC_VERSION);
    manager.add<DefaultBoolCodec>(name);
    manager.add<DefaultStringCodec, FieldDescriptor::TYPE_STRING>(name);
    manager.add<DefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(name);
    manager.add<DefaultEnumCodec>(name);
    manager.add<DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(name);
}

template <>
template <int dummy, typename T, typename... Types>
void DefaultFieldCodecAdder<CODEC_VERSION>::add(FieldCodecManagerLocal& manager)
{
    using namespace CODEC_VERSION_NAMESPACE;
    std::string name = dccl::Codec::default_codec_name(CODEC_VERSION);
    manager.add<DefaultNumericFieldCodec<T>>(name);
    // recurse
    add<0, Types...>(manager);
}

} // namespace dccl

#undef CODEC_VERSION
#undef CODEC_VERSION_NAMESPACE
