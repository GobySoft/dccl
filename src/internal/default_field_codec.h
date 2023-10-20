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

namespace internal
{

//
// Helper functions to reduce copy/paste in set_default_codecs()
//
template <int version> struct DefaultFieldCodecLoader
{
    static_assert(sizeof(DefaultFieldCodecLoader) == 0,
                  "Must use specialization of DefaultFieldCodecLoader");
};

template <int version> struct TimeCodecLoader
{
    static_assert(sizeof(TimeCodecLoader) == 0, "Must use specialization of TimeCodecLoader");
};

template <int version> struct StaticCodecLoader
{
    static_assert(sizeof(StaticCodecLoader) == 0, "Must use specialization of StaticCodecLoader");
};

template <int version> struct PresenceCodecLoader
{
    static_assert(sizeof(PresenceCodecLoader) == 0,
                  "Must use specialization of PresenceCodecLoader");
};

template <int version> struct VarBytesCodecLoader
{
    static_assert(sizeof(VarBytesCodecLoader) == 0,
                  "Must use specialization of VarBytesCodecLoader");
};

template <int version> struct HashCodecLoader
{
    static_assert(sizeof(HashCodecLoader) == 0, "Must use specialization of HashCodecLoader");
};

} // namespace internal
} // namespace dccl
#endif

namespace dccl
{
namespace internal
{
// replace recursion with C++ 17 fold expression when we switch to C++ 17 or newer
template <> struct DefaultFieldCodecLoader<CODEC_VERSION>
{
    // entry
    static void add(FieldCodecManagerLocal& manager)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        using google::protobuf::FieldDescriptor;

        std::string name = ::dccl::Codec::default_codec_name(CODEC_VERSION);
        manager.add<DefaultBoolCodec>(name);
        manager.add<DefaultStringCodec, FieldDescriptor::TYPE_STRING>(name);
        manager.add<DefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(name);
        manager.add<DefaultEnumCodec>(name);
        manager.add<DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(name);

        add<true, double, float, int32, int64, uint32, uint64>(manager);
    }

    // recurse
    template <bool enable, typename T, typename... Types>
    static void add(FieldCodecManagerLocal& manager)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        std::string name = ::dccl::Codec::default_codec_name(CODEC_VERSION);
        manager.add<DefaultNumericFieldCodec<T>>(name);
        // recurse
        add<enable, Types...>(manager);
    }

    // last
    template <bool enable> static void add(FieldCodecManagerLocal& manager) {}
};

template <int version> struct TimeCodecLoader;
template <> struct TimeCodecLoader<CODEC_VERSION>
{
    static void add(FieldCodecManagerLocal& manager,
                    const std::string& name = "dccl.time" + std::to_string(CODEC_VERSION))
    {
        add<true, double, int64, uint64>(manager, name);
    }

    template <bool enable, typename T, typename... Types>
    static void add(FieldCodecManagerLocal& manager, const std::string& name)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        manager.add<TimeCodec<T>>(name);
        add<enable, Types...>(manager, name);
    }
    template <bool enable> static void add(FieldCodecManagerLocal& manager, const std::string& name)
    {
    }
};

template <> struct StaticCodecLoader<CODEC_VERSION>
{
    static void add(FieldCodecManagerLocal& manager,
                    const std::string& name = "dccl.static" + std::to_string(CODEC_VERSION))
    {
        add<true, std::string, double, float, int32, int64, uint32, uint64>(manager, name);
    }

    template <bool enable, typename T, typename... Types>
    static void add(FieldCodecManagerLocal& manager, const std::string& name)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        manager.add<StaticCodec<T>>(name);
        add<enable, Types...>(manager, name);
    }

    template <bool enable> static void add(FieldCodecManagerLocal& manager, const std::string& name)
    {
    }
};

#if CODEC_VERSION >= 3
template <> struct PresenceCodecLoader<CODEC_VERSION>
{
    static void add(FieldCodecManagerLocal& manager)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        using google::protobuf::FieldDescriptor;

        manager.add<PresenceBitCodec<DefaultBoolCodec>>(name_);
        manager.add<PresenceBitCodec<DefaultStringCodec>, FieldDescriptor::TYPE_STRING>(name_);
        manager.add<PresenceBitCodec<DefaultBytesCodec>, FieldDescriptor::TYPE_BYTES>(name_);
        manager.add<PresenceBitCodec<DefaultEnumCodec>>(name_);

        // use normal default meessage codec
        manager.add<DefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(name_);
        add<true, double, float, int32, int64, uint32, uint64>(manager);
    }

    template <bool enable, typename T, typename... Types>
    static void add(FieldCodecManagerLocal& manager)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        std::string name = ::dccl::Codec::default_codec_name(CODEC_VERSION);
        manager.add<PresenceBitCodec<DefaultNumericFieldCodec<T>>>(name_);
        add<enable, Types...>(manager);
    }

    template <bool enable> static void add(FieldCodecManagerLocal& manager) {}

  private:
    static const std::string name_;
};

const std::string PresenceCodecLoader<CODEC_VERSION>::name_{"dccl.presence" +
                                                            std::to_string(CODEC_VERSION)};

template <> struct VarBytesCodecLoader<CODEC_VERSION>
{
    static void add(FieldCodecManagerLocal& manager)
    {
        using namespace CODEC_VERSION_NAMESPACE;
        manager.add<VarBytesCodec>("dccl.var_bytes" + std::to_string(CODEC_VERSION));
    }
};

#endif

#if CODEC_VERSION >= 4
template <> struct HashCodecLoader<CODEC_VERSION>
{
    static void add(FieldCodecManagerLocal& manager, std::set<int> versions = {CODEC_VERSION})
    {
        using namespace CODEC_VERSION_NAMESPACE;
        // backport versions
        for (int v : versions) manager.add<HashCodec>("dccl.hash" + std::to_string(v));
    }
};

#endif

} // namespace internal
} // namespace dccl

#undef CODEC_VERSION
#undef CODEC_VERSION_NAMESPACE
