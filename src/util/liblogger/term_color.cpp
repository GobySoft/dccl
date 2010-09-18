// copyright 2009 t. schneider tes@mit.edu
//
// this file is part of flex-ostream, a terminal display library
// that provides an ostream with both terminal display and file logging
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include "flex_ostream.h"
#include "term_color.h"

std::ostream& goby::tcolor::add_escape_code(std::ostream& os, const std::string& esc_code)
{
    try
    {
        util::FlexOstream& flex = dynamic_cast<util::FlexOstream&>(os);
        return(flex << esc_code);
    }
    catch (const std::bad_cast& e)
    { return(os); }
}
