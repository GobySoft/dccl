#ifndef DCCL_STRING_COMPAT_H
#define DCCL_STRING_COMPAT_H

#include <string>

namespace dccl
{
// Converts string_view-like types from protobuf to std::string.
template <typename StringLike>
inline std::string to_std_string(const StringLike& value)
{
    return std::string(value.data(), value.size());
}
} // namespace dccl

#endif
