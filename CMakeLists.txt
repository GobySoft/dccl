# t. schneider tes@mit.edu 
# base CMakeLists.txt file for dccl
# for help on CMake, see http://www.cmake.org/cmake/help/documentation.html
# or type > man cmake

# this project is intended to be built out of source by using 
# > ./build.sh

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

# FILE GLOB_RECURSE calls should not follow symlinks by default.
cmake_policy(SET CMP0009 NEW)
# MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0042 NEW)

project(dccl)

# boost for units and any (pre C++17)
find_package(Boost 1.62.0 QUIET)
if(Boost_FOUND)
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif()

if(((NOT DEFINED CMAKE_CXX_STANDARD) OR ("${CMAKE_CXX_STANDARD}" STREQUAL "03") OR ("${CMAKE_CXX_STANDARD}" STREQUAL "11")))
  if(Boost_FOUND)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    message ("Setting CMAKE_CXX_STANDARD to 14 as required by DCCL (when Boost is available). If you wish to use a newer standard, set CMAKE_CXX_STANDARD directly")
  else()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    message ("Setting CMAKE_CXX_STANDARD to 17 as required by DCCL (when Boost is not available). If you wish to use a newer standard, set CMAKE_CXX_STANDARD directly. If you wish to use C++14, install Boost (libboost-dev)")
  endif()
endif()

## allows us to write custom modules or modifying existing ones
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

# enable sanitizers (e.g., Address Sanitizer)
find_package(Sanitizers)

# enable testing (CTest)
option(enable_testing "Enable building of tests using CTest (if set to ON, you can run tests with 'make test')" OFF)
if(enable_testing)
  enable_testing()
  include(CTest)
endif()

option(build_apps "Build supporting tools and other binary applications" ON)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/version.txt DCCL_VERSION_STRING)
string(REGEX REPLACE "^([0-9]+).*$" "\\1" DCCL_VERSION_MAJOR "${DCCL_VERSION_STRING}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$" "\\1" DCCL_VERSION_MINOR  "${DCCL_VERSION_STRING}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9~a-z]+).*$" "\\1" DCCL_VERSION_PATCH "${DCCL_VERSION_STRING}")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
   message("Compiling in Git source tree.")
   include(today)
   today(DCCL_VERSION_DATE)  
   execute_process(COMMAND git rev-parse --short HEAD
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     OUTPUT_VARIABLE DCCL_LAST_REV)
   string(STRIP ${DCCL_LAST_REV} DCCL_LAST_REV)

   string(REPLACE "~" "_" DCCL_GIT_VERSION ${DCCL_VERSION_MAJOR}.${DCCL_VERSION_MINOR}.${DCCL_VERSION_PATCH}) 

   execute_process(COMMAND git rev-list ${DCCL_GIT_VERSION}..HEAD --count
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     OUTPUT_VARIABLE DCCL_REVS_SINCE_TAG)
   string(STRIP ${DCCL_REVS_SINCE_TAG} DCCL_REVS_SINCE_TAG)
      
   execute_process(COMMAND git diff-index --quiet HEAD
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     RESULT_VARIABLE DCCL_DIRTY_REV)
   if(DCCL_DIRTY_REV EQUAL 0)
     set(DCCL_DIRTY_REV_STRING "")
   else()
     set(DCCL_DIRTY_REV_STRING "-dirty")
   endif()
   set(DCCL_VERSION_PATCH "${DCCL_VERSION_PATCH}+${DCCL_REVS_SINCE_TAG}+git${DCCL_LAST_REV}${DCCL_DIRTY_REV_STRING}")
   
else()
   message("Compiling from release tarball.")
   set(DCCL_VERSION_DATE "2023.03.21")
endif()


set(DCCL_VERSION "${DCCL_VERSION_MAJOR}.${DCCL_VERSION_MINOR}.${DCCL_VERSION_PATCH}")


include(today)
today(DCCL_COMPILE_DATE)

# give Dccl 2 series a few more soversions
set(DCCL_SOVERSION "31")

# fetch all the local directories for generated code
get_filename_component(dccl_SRC_DIR src ABSOLUTE)
set(dccl_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
get_filename_component(dccl_SCRIPTS_DIR scripts ABSOLUTE)

get_filename_component(dccl_LIB_DIR ${dccl_BUILD_DIR}/lib ABSOLUTE)
get_filename_component(dccl_BIN_DIR ${dccl_BUILD_DIR}/bin ABSOLUTE)
get_filename_component(dccl_INC_DIR ${dccl_BUILD_DIR}/include ABSOLUTE)
get_filename_component(dccl_SHARE_DIR ${dccl_BUILD_DIR}/share ABSOLUTE)

get_filename_component(dccl_PYTHON_DIR python ABSOLUTE)

if(CMAKE_CROSSCOMPILING)
  message("DCCL is cross-compiling. Target library architecture: ${CMAKE_LIBRARY_ARCHITECTURE}")
  find_program(dccl_PROTOC_GEN_EXEC protoc-gen-dccl)
  if(dccl_PROTOC_GEN_EXEC)
    get_filename_component(dccl_DEFAULT_EXEC_DIR ${dccl_PROTOC_GEN_EXEC} DIRECTORY)
  else()
    message(FATAL_ERROR "Failed to find protoc-gen-dccl for cross-compiling")
  endif()
else()
  set(dccl_DEFAULT_EXEC_DIR "${dccl_BIN_DIR}")
endif()
  
set(dccl_EXEC_DIR "${dccl_DEFAULT_EXEC_DIR}" CACHE STRING "Directory where executable DCCL code resides. Change for cross-compilation.")

# symlink these into the build directory for backwards compatibility
option(ADD_SYMLINKS "Add symlinks to root directory for backwards compatibility." ON)
if (ADD_SYMLINKS)
  get_filename_component(dccl_FORMER_LIB_DIR lib ABSOLUTE)
  get_filename_component(dccl_FORMER_BIN_DIR bin ABSOLUTE)
  get_filename_component(dccl_FORMER_INC_DIR include ABSOLUTE)
  get_filename_component(dccl_FORMER_SHARE_DIR share ABSOLUTE)
  add_custom_command(OUTPUT LibLink COMMAND ${CMAKE_COMMAND} -E create_symlink ${dccl_LIB_DIR} ${dccl_FORMER_LIB_DIR})
  add_custom_target(lib_link ALL DEPENDS LibLink)
  add_custom_command(OUTPUT BinLink COMMAND ${CMAKE_COMMAND} -E create_symlink ${dccl_BIN_DIR} ${dccl_FORMER_BIN_DIR})
  add_custom_target(bin_link ALL DEPENDS BinLink)
  add_custom_command(OUTPUT IncludeLink COMMAND ${CMAKE_COMMAND} -E create_symlink ${dccl_INC_DIR} ${dccl_FORMER_INC_DIR})
  add_custom_target(inc_link ALL DEPENDS IncludeLink)
  add_custom_command(OUTPUT ShareLink COMMAND ${CMAKE_COMMAND} -E create_symlink ${dccl_SHARE_DIR} ${dccl_FORMER_SHARE_DIR})
  add_custom_target(share_link ALL DEPENDS ShareLink)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall") # -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall") # -fprofile-arcs -ftest-coverage")

## set type of libraries
option(make_static_libs "Build static libraries instead of shared." OFF)

if(make_static_libs)
  set(BUILD_SHARED_LIBS OFF)
else()
  set(BUILD_SHARED_LIBS ON)
endif()

## set the cmake defaults for libraries and binaries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dccl_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dccl_BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dccl_LIB_DIR})


## set instructions for `make install`

# override library version for debian sbuild cross-builds
if(CMAKE_CROSSCOMPILING)
  if (EXISTS "/etc/debian_version")
    set(CMAKE_INSTALL_LIBDIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
  endif()
endif()

include(GNUInstallDirs)

install(DIRECTORY ${dccl_SHARE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dccl)

install(DIRECTORY ${dccl_INC_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN  "*.h" PATTERN "*.hpp" PATTERN "*.proto" PATTERN "test*" EXCLUDE)
install(DIRECTORY ${dccl_SRC_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dccl
  FILES_MATCHING PATTERN  "*.h" PATTERN "*.hpp" PATTERN "*.proto" PATTERN "test*" EXCLUDE)

install(EXPORT dccl-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dccl)

message("Installation directories set to: \n\tlibraries: ${CMAKE_INSTALL_FULL_LIBDIR}\n\tbinaries: ${CMAKE_INSTALL_FULL_BINDIR}\n\theaders: ${CMAKE_INSTALL_FULL_INCLUDEDIR}")

## let cmake know where the headers are
include_directories(${dccl_INC_DIR})

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${dccl_INC_DIR};${dccl_FORMER_BIN_DIR};${dccl_FORMER_LIB_DIR};${dccl_FORMER_INC_DIR};${dccl_FORMER_SHARE_DIR};${dccl_PYTHON_DIR}/dccl/option_extensions_pb2.py;${dccl_SRC_DIR}/share/examples/quickstart_navreport/navreport_pb2.py;${dccl_SRC_DIR}/share/examples/quickstart_navreport/navreport_pb2.pyc"
  )

find_package(ProtobufDCCL REQUIRED)
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIRS})

message("Google Protocol Buffers Version: ${PROTOC_VERSION}")

# optional
## cryptopp
find_package(Cryptopp QUIET)
set(CRYPTOPP_DOC_STRING "Enable cryptography (requires libcrypto++-dev: http://www.cryptopp.com)")
if(CRYPTOPP_FOUND)
  option(enable_cryptography ${CRYPTOPP_DOC_STRING} ON)
else()
  option(enable_cryptography ${CRYPTOPP_DOC_STRING} OFF)
  message(">> setting enable_cryptography to OFF ... if you need this functionality: 1) install libcrypto++-dev; 2) run cmake -Denable_cryptography=ON")
endif()

if(enable_cryptography)
  find_package(Cryptopp REQUIRED)
  set(DCCL_HAS_CRYPTOPP "1")
  include_directories(SYSTEM ${Cryptopp_INCLUDE_DIRS})
else()
  set(DCCL_HAS_CRYPTOPP "0")
endif()

## b64 for base64 functions
find_package(B64 QUIET)
set(B64_DOC_STRING "Enable base64 functionality (requires libb64-dev: http://libb64.sourceforge.net/")
if(B64_FOUND)
  option(enable_b64 ${B64_DOC_STRING} ON)
else()
  option(enable_b64 ${B64_DOC_STRING} OFF)
  message(">> setting enable_b64 to OFF ... if you need this functionality: 1) install libb64-dev; 2) run cmake -Denable_b64=ON")
endif()

if(enable_b64)
  find_package(B64 REQUIRED)
  set(DCCL_HAS_B64 "1")
  include_directories(SYSTEM ${B64_INCLUDE_DIRS})
else()
  set(DCCL_HAS_B64 "0")
endif()


## LUA for dynamic conditionals
find_package(Lua QUIET)
set(LUA_DOC_STRING "Enable LUA functionality (requires liblua5.3-dev")
if(LUA_FOUND)
  option(enable_lua ${LUA_DOC_STRING} ON)
else()
  option(enable_lua ${LUA_DOC_STRING} OFF)
  message(">> setting enable_lua to OFF ... if you need this functionality: 1) install liblua5.3-dev; 2) run cmake -Denable_lua=ON")
endif()

if(enable_lua)
  find_package(Lua REQUIRED)
  set(DCCL_HAS_LUA "1")
  include_directories(SYSTEM ${LUA_INCLUDE_DIR})
else()
  set(DCCL_HAS_LUA "0")
endif()

option(enable_thread_safety "Enable DCCL thread safety (uses std::mutex)" ON)
if(enable_thread_safety)
  set(DCCL_HAS_THREAD_SUPPORT "1")
  # Protobuf uses pthread so we need to link it too (if it's here)
  find_package(Threads)
else()
  set(DCCL_HAS_THREAD_SUPPORT "0")
endif()

## boost for units
set(UNITS_DOC_STRING "Enable static unit-safety functionality (requires Boost)")
if(Boost_FOUND)
  option(enable_units ${UNITS_DOC_STRING} ON) 
else()
  option(enable_units ${UNITS_DOC_STRING} OFF)
endif()

## clang-tidy
option(enable_clang_tidy "Run code through clang-tidy" OFF)

if(enable_clang_tidy)
  # Find clang-tidy
  find_program(CLANG_TIDY_BIN NAMES "clang-tidy" DOC "Path to clang-tidy executable")
  if(CLANG_TIDY_BIN)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_BIN}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_BIN}" "-fix" "-checks=modernize-use-override,modernize-deprecated-headers,misc-static-assert,misc-unused-using-decls,misc-unused-parameters,modernize-pass-by-value,modernize-use-using,modernize-use-emplace,modernize-use-nullptr,modernize-use-equals-default,modernize-use-equals-delete,modernize-use-default-member-init,modernize-use-auto,modernize-raw-string-literal,modernize-make-shared,modernize-make-unique,modernize-loop-convert,readability-mismatched-tags,readability-make-member-function-const" "-header-filter=^.*[^(pb)]\\.h$")
  else()
    message(FATAL_ERROR "clang-tidy not found")
  endif()
endif()


set(DCCL_DEBUG_OPTIONS -Wall -Wno-sign-compare -Wno-unknown-warning-option)
add_compile_options("$<$<CONFIG:DEBUG>:${DCCL_DEBUG_OPTIONS}>")
add_compile_options("$<$<CXX_COMPILER_ID:Clang>:-Wdocumentation>")

## configure to build/include/dccl
file(GLOB_RECURSE COPY_INCLUDE_FILES RELATIVE ${dccl_SRC_DIR} src/*.h src/*.proto src/*.hpp)
foreach(I ${COPY_INCLUDE_FILES})
  configure_file(${dccl_SRC_DIR}/${I} ${dccl_INC_DIR}/dccl/${I} COPYONLY)
endforeach()

file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${dccl_SRC_DIR} src/def.h src/version.h)
foreach(I ${INCLUDE_FILES})
  configure_file(${dccl_SRC_DIR}/${I} ${dccl_INC_DIR}/dccl/${I} @ONLY)
endforeach()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dccl.h ${dccl_INC_DIR}/dccl.h @ONLY)

configure_file(${dccl_PYTHON_DIR}/setup.py.in ${dccl_PYTHON_DIR}/setup.py @ONLY)


## copy to build/share
file(GLOB SHARE_FILES RELATIVE ${dccl_SRC_DIR} src/share/*)
foreach(I ${SHARE_FILES})
  file(COPY ${dccl_SRC_DIR}/${I} DESTINATION ${dccl_SHARE_DIR})
endforeach()

## start adding subdirectories
add_subdirectory(src)

