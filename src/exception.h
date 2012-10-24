// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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



#ifndef Exception20100812H
#define Exception20100812H

#include <stdexcept>

namespace dccl
{
    /// \brief Exception class for libdccl
    class Exception : public std::runtime_error {
      public:
      Exception(const std::string& s)
          : std::runtime_error(s)
        { }

    };

    // used to signal null value in field codecs
    class NullValueException : public Exception
    {
      public:
      NullValueException()
          : Exception("NULL Value")
        { }    
    };
        
}


#endif

