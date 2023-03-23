// Copyright 2009-2023 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
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

// clang-format off
#define DCCL_THREAD_SUPPORT @DCCL_HAS_THREAD_SUPPORT@
// clang-format on

#if DCCL_THREAD_SUPPORT
#include <mutex>
#include <thread>
#include <atomic>
namespace dccl
{
extern std::recursive_mutex g_dynamic_protobuf_manager_mutex;
extern std::recursive_mutex g_dlog_mutex;
} // namespace dccl

#define LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX \
    std::lock_guard<std::recursive_mutex> l(g_dynamic_protobuf_manager_mutex);
#define LOCK_DLOG_MUTEX std::lock_guard<std::recursive_mutex> l(g_dlog_mutex);
#else
#define LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
#define LOCK_DLOG_MUTEX
#endif
