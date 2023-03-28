#ifndef DCCLFIELDCODECDATAH
#define DCCLFIELDCODECDATAH

#include "../dynamic_conditions.h"

#include "field_codec_message_stack.h"

#include <typeindex>

namespace google
{
namespace protobuf
{
class Message;
class Descriptor;
} // namespace protobuf
} // namespace google

namespace dccl
{
namespace internal
{
// Data shared amongst all the FieldCodecs for a single message
struct CodecData
{
    MessagePart part_{dccl::UNKNOWN};
    bool strict_{false};
    const google::protobuf::Message* root_message_{nullptr};
    const google::protobuf::Descriptor* root_descriptor_{nullptr};
    MessageStackData message_data_;
    DynamicConditions dynamic_conditions_;

    template <typename FieldCodecType>
    void set_codec_specific_data(std::shared_ptr<dccl::any> data)
    {
        codec_specific_[std::type_index(typeid(FieldCodecType))] = data;
    }

    template <typename FieldCodecType> std::shared_ptr<dccl::any> codec_specific_data()
    {
        return codec_specific_.at(std::type_index(typeid(FieldCodecType)));
    }

    template <typename FieldCodecType> bool has_codec_specific_data()
    {
        return codec_specific_.count(std::type_index(typeid(FieldCodecType)));
    }

  private:
    std::map<std::type_index, std::shared_ptr<dccl::any>> codec_specific_;
};
} // namespace internal
} // namespace dccl

#endif
