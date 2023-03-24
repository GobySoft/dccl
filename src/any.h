// Copyright 2009-2022:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Davide Fenucci <davfen@noc.ac.uk>
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
#ifndef DCCLAny
#define DCCLAny

#include "common.h"

#ifdef DCCL_HAS_CPP17
#include <any>
namespace dccl
{
using any = std::any;
template <class T> T any_cast(const any& operand) { return std::any_cast<T>(operand); }
template <class T> T any_cast(any& operand) { return std::any_cast<T>(operand); }
template <class T> T any_cast(any&& operand) { return std::any_cast<T>(operand); }
template <class T> const T* any_cast(const any* operand) noexcept
{
    return std::any_cast<T>(operand);
}
template <class T> T* any_cast(any* operand) noexcept { return std::any_cast<T>(operand); }
using bad_any_cast = std::bad_any_cast;
inline bool is_empty(const any& a) { return !a.has_value(); }
} // namespace dccl
#else
#include <boost/any.hpp>
namespace dccl
{
using any = boost::any;
template <class T> T any_cast(const any& operand) { return boost::any_cast<T>(operand); }
template <class T> T any_cast(any& operand) { return boost::any_cast<T>(operand); }
template <class T> T any_cast(any&& operand) { return boost::any_cast<T>(operand); }
template <class T> const T* any_cast(const any* operand) noexcept
{
    return boost::any_cast<T>(operand);
}
template <class T> T* any_cast(any* operand) noexcept { return boost::any_cast<T>(operand); }
using bad_any_cast = boost::bad_any_cast;
inline bool is_empty(const any& a) { return a.empty(); }
} // namespace dccl
#endif

#endif
