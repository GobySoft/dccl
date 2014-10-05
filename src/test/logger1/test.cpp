// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Applications
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

#include "dccl/logger.h"
using namespace dccl::test;

/// asserts false if called - used for testing proper short-circuiting of logger calls
inline std::ostream& stream_assert(std::ostream & os)
{
    bool failed_to_short_circuit_logging_statement = false;
    assert(failed_to_short_circuit_logging_statement);
    return os;
}


void info(const std::string& log_message,
          dccl::logger::Verbosity verbosity,
          dccl::logger::Group group)
{
   printf("%s\n", log_message.c_str());
}

int main(int argc, char* argv[])
{
    using dccl::dlog;
    using namespace dccl::logger;

    std::cout << "attaching info() to DEBUG3+" << std::endl;
    dlog.connect(DEBUG3_PLUS, &info);
    dlog.is(DEBUG3) && dlog << "debug3 ok" << std::endl;
    dlog.is(DEBUG2) && dlog << "debug2 ok" << std::endl;
    dlog.is(DEBUG1) && dlog << "debug1 ok" << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    

    std::cout << "attaching info() to nothing" << std::endl;
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << stream_assert << std::endl;
    dlog.is(INFO) && dlog << stream_assert << std::endl;
    dlog.is(WARN) && dlog << stream_assert << std::endl;
    
    std::cout << "attaching info() to WARN+" << std::endl;
    dlog.connect(WARN_PLUS, &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << stream_assert << std::endl;
    dlog.is(INFO) && dlog << stream_assert << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);

    std::cout << "attaching info() to INFO+" << std::endl;
    dlog.connect(INFO_PLUS, &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << stream_assert << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    
    
    std::cout << "attaching info() to DEBUG1+" << std::endl;
    dlog.connect(DEBUG1_PLUS, &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << "debug1 ok" << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    

    std::cout << "attaching info() to DEBUG2+" << std::endl;
    dlog.connect(DEBUG2_PLUS, &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << "debug2 ok" << std::endl;
    dlog.is(DEBUG1) && dlog << "debug1 ok" << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    

    
    std::cout << "All tests passed." << std::endl;

}

