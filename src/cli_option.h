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

#ifndef CLIOPTION20140107H
#define CLIOPTION20140107H

#include <getopt.h>

#include <sstream>
#include <iomanip>

namespace dccl
{
    class Option
    {
      public:
        Option(char shortname,
               const char* longname,
               int has_argument,
               const std::string& description)
            : description_(description)
        {
            c_opt_.name = longname;
            
            c_opt_.has_arg = has_argument;
            c_opt_.flag = 0;
            c_opt_.val = shortname;
        }

        option c_opt() const { return c_opt_; }
        std::string opt_code() const 
        {
            std::string opt_code;
            if(c_opt_.val == 0)
                return opt_code;

            opt_code += std::string(1, c_opt_.val);
            if(c_opt_.has_arg == no_argument)
                return opt_code;
            else if(c_opt_.has_arg == required_argument)
                return opt_code + ":";
            else if(c_opt_.has_arg == optional_argument)
                return opt_code + "::";
            else
                return "";
            
        }    

        std::string usage() const 
        {
            std::stringstream usage; 
            if(c_opt_.val != 0)
                usage << "-" << (char)c_opt_.val << ", ";
            usage << "--" << c_opt_.name;
            usage << std::string(std::max(1, (int)(20-usage.str().size())), ' ') << description_;
            return usage.str();
        }
    
        static void convert_vector(const std::vector<Option>& options, std::vector<option>* c_options, std::string* opt_string)
        {
            for(int i = 0, n = options.size(); i < n; ++i)
            {
                c_options->push_back(options[i].c_opt());
                *opt_string += options[i].opt_code();
            }
            option zero = { 0, 0, 0, 0 };
            c_options->push_back(zero);
        }
    
      private:
        option c_opt_;
        std::string description_;
    };
}

#endif

