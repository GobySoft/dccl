// Copyright 2009-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
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
#ifndef DCCLFIELDCODECDEFAULTMESSAGEV320140421H
#define DCCLFIELDCODECDEFAULTMESSAGEV320140421H

#include "../field_codec.h"
#include "../field_codec_manager.h"

#include "dccl/option_extensions.pb.h"

namespace dccl
{
namespace v3
{
/// \brief Provides the default codec for encoding a base Google Protobuf message or an embedded message by calling the appropriate field codecs for every field.
class DefaultMessageCodec : public FieldCodecBase
{
  private:
    void any_encode(Bitset* bits, const dccl::any& wire_value) override;
    void any_decode(Bitset* bits, dccl::any* wire_value) override;
    unsigned max_size() override;
    unsigned min_size() override;
    unsigned any_size(const dccl::any& wire_value) override;

    std::shared_ptr<FieldCodecBase> find(const google::protobuf::FieldDescriptor* field_desc)
    {
        return manager().find(field_desc, has_codec_group(), codec_group());
    }

    bool is_optional() { return this_field() && this_field()->is_optional(); }

    void validate() override;
    std::string info() override;
    std::size_t hash() override;
    bool check_field(const google::protobuf::FieldDescriptor* field);

    struct Size
    {
        static void repeated(std::shared_ptr<FieldCodecBase> codec, unsigned* return_value,
                             const std::vector<dccl::any>& field_values,
                             const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_size_repeated(return_value, field_values, field_desc);
        }

        static void single(std::shared_ptr<FieldCodecBase> codec, unsigned* return_value,
                           const dccl::any& field_value,
                           const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_size(return_value, field_value, field_desc);
        }
    };

    struct Encoder
    {
        static void repeated(std::shared_ptr<FieldCodecBase> codec, Bitset* return_value,
                             const std::vector<dccl::any>& field_values,
                             const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_encode_repeated(return_value, field_values, field_desc);
        }

        static void single(std::shared_ptr<FieldCodecBase> codec, Bitset* return_value,
                           const dccl::any& field_value,
                           const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_encode(return_value, field_value, field_desc);
        }
    };

    struct MaxSize
    {
        static void field(std::shared_ptr<FieldCodecBase> codec, unsigned* return_value,
                          const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_max_size(return_value, field_desc);
        }
    };

    struct MinSize
    {
        static void field(std::shared_ptr<FieldCodecBase> codec, unsigned* return_value,
                          const google::protobuf::FieldDescriptor* field_desc)
        {
            // defer minimum size calculation for dynamic conditions (since omit == 0)
            // this may provide an incorrect min_size() value for reporting but allows
            // use of index based lua scripts in dynamic conditions
            DynamicConditions& dc = codec->dynamic_conditions(field_desc);
            if (dc.has_omit_if() || dc.has_required_if())
                *return_value = 0;
            else
                codec->field_min_size(return_value, field_desc);
        }
    };

    struct Validate
    {
        static void field(std::shared_ptr<FieldCodecBase> codec, bool* return_value,
                          const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_validate(return_value, field_desc);
        }
    };

    struct Info
    {
        static void field(std::shared_ptr<FieldCodecBase> codec, std::stringstream* return_value,
                          const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_info(return_value, field_desc);
        }
    };

    struct Hash
    {
        static void field(std::shared_ptr<FieldCodecBase> codec, std::size_t* return_value,
                          const google::protobuf::FieldDescriptor* field_desc)
        {
            codec->field_hash(return_value, field_desc);
        }
    };

    template <typename Action, typename ReturnType>
    void traverse_descriptor(ReturnType* return_value)
    {
        const google::protobuf::Descriptor* desc = FieldCodecBase::this_descriptor();

        for (int i = 0, n = desc->field_count(); i < n; ++i)
        {
            const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

            if (!check_field(field_desc))
                continue;

            Action::field(find(field_desc), return_value, field_desc);
        }
    }

    template <typename Action, typename ReturnType>
    ReturnType traverse_const_message(const dccl::any& wire_value)
    {
        try
        {
            ReturnType return_value = ReturnType();

            const auto* msg = dccl::any_cast<const google::protobuf::Message*>(wire_value);
            const google::protobuf::Descriptor* desc = msg->GetDescriptor();
            const google::protobuf::Reflection* refl = msg->GetReflection();

            for (int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

                if (!check_field(field_desc))
                    continue;

                std::shared_ptr<FieldCodecBase> codec = find(field_desc);
                std::shared_ptr<internal::FromProtoCppTypeBase> helper =
                    manager().type_helper().find(field_desc);

                if (field_desc->is_repeated())
                {
                    std::vector<dccl::any> field_values;
                    for (int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                        field_values.push_back(helper->get_repeated_value(field_desc, *msg, j));

                    Action::repeated(codec, &return_value, field_values, field_desc);
                }
                else
                {
                    // singular field dynamic conditions - repeated fields handled in any_encode_repeated
                    DynamicConditions& dc = dynamic_conditions(field_desc);
                    if (dc.has_omit_if())
                    {
                        // expensive, so don't do this unless we're going to use it
                        dc.regenerate(this_message(), root_message());
                        if (dc.omit())
                            continue;
                    }

                    Action::single(codec, &return_value, helper->get_value(field_desc, *msg),
                                   field_desc);
                }
            }
            return return_value;
        }
        catch (dccl::bad_any_cast& e)
        {
            throw(Exception("Bad type given to traverse const, expecting const "
                            "google::protobuf::Message*, got " +
                            std::string(wire_value.type().name())));
        }
    }
};

} // namespace v3
} // namespace dccl

//encode, size, etc.

#endif
