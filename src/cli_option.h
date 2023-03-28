// Copyright 2014-2017:
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
#ifndef CLIOPTION20140107H
#define CLIOPTION20140107H

#include <getopt.h>

#include <iomanip>
#include <sstream>
#include <utility>

namespace dccl
{
/// Represents a command line option
class Option
{
  public:
    /// \brief Create a command line option
    ///
    /// \param shortname Single character representation (e.g. 'v' for "-v")
    /// \param longname Full representation (e.g. "verbose" for "--verbose")
    /// \param has_argument Does the parameter take an argument?
    /// \param description Human description for the --help option
    Option(char shortname, const char* longname, int has_argument, std::string description)
        : description_(std::move(description))
    {
        c_opt_.name = longname;

        c_opt_.has_arg = has_argument;
        c_opt_.flag = nullptr;
        c_opt_.val = shortname;
    }

    /// \return Equivalent option from <getopt.h>
    option c_opt() const { return c_opt_; }

    /// \return option code from <getopt.h> used in getopt_long()
    std::string opt_code() const
    {
        std::string opt_code;
        if (c_opt_.val == 0)
            return opt_code;

        opt_code += std::string(1, c_opt_.val);
        if (c_opt_.has_arg == no_argument)
            return opt_code;
        else if (c_opt_.has_arg == required_argument)
            return opt_code + ":";
        else if (c_opt_.has_arg == optional_argument)
            return opt_code + "::";
        else
            return "";
    }

    /// \return String giving usage for the --help option.
    std::string usage() const
    {
        std::stringstream usage;
        if (c_opt_.val != 0)
            usage << "-" << (char)c_opt_.val << ", ";
        usage << "--" << c_opt_.name;
        usage << std::string(std::max(1, (int)(20 - usage.str().size())), ' ') << description_;
        return usage.str();
    }

    /// \brief Convert a vector of Options into a vector of options (from getopt.h) and an opt_string, suitable for use in getopt_long()
    static void convert_vector(const std::vector<Option>& options, std::vector<option>* c_options,
                               std::string* opt_string)
    {
        for (const auto& option : options)
        {
            c_options->push_back(option.c_opt());
            *opt_string += option.opt_code();
        }
        option zero = {nullptr, 0, nullptr, 0};
        c_options->push_back(zero);
    }

  private:
    option c_opt_;
    std::string description_;
};
} // namespace dccl

#endif
