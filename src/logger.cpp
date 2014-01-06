// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

#include <ctime>

#include "dccl/logger.h"

dccl::Logger dccl::dlog;

int dccl::LogBuffer::sync() {
    // all but last one
    while(buffer_.size() > 1) {
        display(buffer_.front());
        buffer_.pop_front();
    }
    verbosity_ = logger::INFO;
    group_ = logger::GENERAL;
    
    return 0;
}

int dccl::LogBuffer::overflow(int c) {
    if (c == EOF) { return c; }
    else if(c == '\n') { buffer_.push_back(std::string()); }
    else { buffer_.back().push_back(c); }
    return c;
}

void dccl::to_ostream(const std::string& msg, dccl::logger::Verbosity vrb,
                      dccl::logger::Group grp, std::ostream* os,
                      bool add_timestamp)
{
    std::string grp_str;
    switch(grp)
    {
        default:
        case logger::GENERAL: break;
        case logger::ENCODE: grp_str = "{encode}: "; break;
        case logger::DECODE: grp_str = "{decode}: "; break;
    }
    
    std::time_t now = std::time(0);
    std::tm* t = std::gmtime(&now);

    if(add_timestamp)
    {
        *os << "[ " << (t->tm_year+1900) << "-"
            << std::setw(2) << std::setfill('0') << (t->tm_mon+1) << "-"
            << std::setw(2) << std::setfill('0') << t->tm_mday
            << " "
            << std::setw(2) << std::setfill('0') << t->tm_hour << ":"
            << std::setw(2) << std::setfill('0') << t->tm_min << ":"
            << std::setw(2) << std::setfill('0') << t->tm_sec << " ]: ";
    }
    
    *os << grp_str << msg << std::endl;
    
}
