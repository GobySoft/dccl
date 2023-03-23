// Copyright 2012-2017:
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
#include <ctime>

#include "dccl/logger.h"

dccl::Logger dccl::dlog;

int dccl::internal::LogBuffer::sync()
{
#if DCCL_THREAD_SUPPORT
    if (verbosity() == logger::UNKNOWN)
    {
        std::cerr
            << "Must use 'dlog.is(...) && dlog << ... << std::endl;' expression when running in "
               "thread safe mode to allow the dlog mutex to be correctly locked and unlocked. "
               "Offending line: "
            << buffer_.front() << std::endl;
        exit(EXIT_FAILURE);
    }

#endif

    // all but last one
    while (buffer_.size() > 1)
    {
        display(buffer_.front());
        buffer_.pop_front();
    }

    if (!verbosity_.empty())
        verbosity_.pop();
    if (!group_.empty())
        group_.pop();

#if DCCL_THREAD_SUPPORT
    g_dlog_mutex.unlock();
#endif
    return 0;
}

int dccl::internal::LogBuffer::overflow(int c)
{
    if (c == EOF)
    {
        return c;
    }
    else if (c == '\n')
    {
        buffer_.push_back(std::string());
    }
    else
    {
        buffer_.back().push_back(c);
    }
    return c;
}

void dccl::to_ostream(const std::string& msg, dccl::logger::Verbosity vrb, dccl::logger::Group grp,
                      std::ostream* os, bool add_timestamp)
{
    std::string grp_str;
    switch (grp)
    {
        default:
        case logger::GENERAL: break;
        case logger::ENCODE: grp_str = "{encode}: "; break;
        case logger::DECODE: grp_str = "{decode}: "; break;
        case logger::SIZE: grp_str = "{size}: "; break;
    }

    std::time_t now = std::time(0);
    std::tm* t = std::gmtime(&now);

    if (add_timestamp)
    {
        *os << "[ " << (t->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
            << (t->tm_mon + 1) << "-" << std::setw(2) << t->tm_mday << " " << std::setw(2)
            << t->tm_hour << ":" << std::setw(2) << t->tm_min << ":" << std::setw(2) << t->tm_sec
            << " ]: " << std::setfill(' ');
    }

    *os << grp_str << msg << std::endl;
}
