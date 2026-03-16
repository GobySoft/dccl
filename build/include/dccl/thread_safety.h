// Copyright 2012-2023:
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
#include "dccl/def.h"

#if DCCL_THREAD_SUPPORT
#include <atomic>
#include <mutex>
#include <thread>
namespace dccl
{
extern std::recursive_mutex g_dynamic_protobuf_manager_mutex;
extern std::recursive_mutex g_dlog_mutex;
} // namespace dccl

#define DCCL_LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX \
    std::lock_guard<std::recursive_mutex> l(dccl::g_dynamic_protobuf_manager_mutex);
#define DCCL_LOCK_DLOG_MUTEX std::lock_guard<std::recursive_mutex> l(dccl::g_dlog_mutex);
#else
// no op
#define DCCL_LOCK_DYNAMIC_PROTOBUF_MANAGER_MUTEX
#define DCCL_LOCK_DLOG_MUTEX
#endif
