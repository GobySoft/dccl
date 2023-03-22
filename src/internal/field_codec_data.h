#ifndef DCCLFIELDCODECDATAH
#define DCCLFIELDCODECDATAH

#include "dccl/dynamic_conditions.h"

#include "field_codec_message_stack.h"

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
};
} // namespace internal
} // namespace dccl

#endif
