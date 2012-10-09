// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#include "dccl/logger.h"

/// asserts false if called - used for testing proper short-circuiting of logger calls
inline std::ostream& stream_assert(std::ostream & os)
{
    bool failed_to_short_circuit_logging_statement = false;
    assert(failed_to_short_circuit_logging_statement);
}


void info(const std::string& log_message,
          dccl::logger::Verbosity verbosity)
{
   printf("%s\n", log_message.c_str());
}

int main(int argc, char* argv[])
{
    using dccl::dlog;
    using namespace dccl::logger;

    std::cout << "attaching info() to DEBUG3+" << std::endl;
    dlog.connect(dlog.verbosity_plus_higher(DEBUG3), &info);
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
    dlog.connect(dlog.verbosity_plus_higher(WARN), &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << stream_assert << std::endl;
    dlog.is(INFO) && dlog << stream_assert << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);

    std::cout << "attaching info() to INFO+" << std::endl;
    dlog.connect(dlog.verbosity_plus_higher(INFO), &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << stream_assert << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    
    
    std::cout << "attaching info() to DEBUG1+" << std::endl;
    dlog.connect(dlog.verbosity_plus_higher(DEBUG1), &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG1) && dlog << "debug1 ok" << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    

    std::cout << "attaching info() to DEBUG2+" << std::endl;
    dlog.connect(dlog.verbosity_plus_higher(DEBUG2), &info);
    dlog.is(DEBUG3) && dlog << stream_assert << std::endl;
    dlog.is(DEBUG2) && dlog << "debug2 ok" << std::endl;
    dlog.is(DEBUG1) && dlog << "debug1 ok" << std::endl;
    dlog.is(INFO) && dlog << "verbose ok" << std::endl;
    dlog.is(WARN) && dlog << "warn ok" << std::endl;
    dlog.disconnect(ALL);    

    
    std::cout << "All tests passed." << std::endl;

}

