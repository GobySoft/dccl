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

#include "dccl/logger.h"

dccl::Logger dccl::dlog;

int dccl::LogBuffer::sync() {
    // all but last one
    while(buffer_.size() > 1) {
        display(buffer_.front());
        buffer_.pop_front();
    }
    verbosity_ = logger::INFO;

    return 0;
}

int dccl::LogBuffer::overflow(int c) {
    if (c == EOF) { return c; }
    else if(c == '\n') { buffer_.push_back(std::string()); }
    else { buffer_.back().push_back(c); }
    return c;
}
