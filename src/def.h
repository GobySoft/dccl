// Copyright 2009-2017:
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

#ifndef DEF20230327H
#define DEF20230327H

// sets CMake defined compile-time definitions (used with configure_file())

// clang-format off
#define DCCL_HAS_CRYPTOPP @DCCL_HAS_CRYPTOPP@
#define DCCL_HAS_B64 @DCCL_HAS_B64@
#define DCCL_HAS_LUA @DCCL_HAS_LUA@
#define DCCL_THREAD_SUPPORT @DCCL_HAS_THREAD_SUPPORT@
// clang-format on

#endif
