#ifndef VERSION20110304H
#define VERSION20110304H

#include <string>

#define DCCL_VERSION_MAJOR @DCCL_VERSION_MAJOR@
#define DCCL_VERSION_MINOR @DCCL_VERSION_MINOR@
#define DCCL_VERSION_PATCH @DCCL_VERSION_PATCH@

namespace dccl
{
    const std::string VERSION_STRING = "@DCCL_VERSION@";
    const std::string VERSION_DATE = "@DCCL_VERSION_DATE@";
    const std::string COMPILE_DATE = "@DCCL_COMPILE_DATE@";
}

#endif
