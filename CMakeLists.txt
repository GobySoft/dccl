# t. schneider tes@mit.edu 
# base CMakeLists.txt file for dccl
# for help on CMake, see http://www.cmake.org/cmake/help/documentation.html
# or type > man cmake

# this project is intended to be built out of source by using 
# > ./build.sh

cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

# FILE GLOB_RECURSE calls should not follow symlinks by default.
cmake_policy(SET CMP0009 NEW)

project(dccl)

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

set(DCCL_VERSION_MAJOR "3")
set(DCCL_VERSION_MINOR "0")
set(DCCL_VERSION_PATCH "10")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
   message("Compiling in Git source tree.")
   include(today)
   today(DCCL_VERSION_DATE)  
   execute_process(COMMAND git rev-parse --short HEAD
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     OUTPUT_VARIABLE DCCL_LAST_REV)
   string(STRIP ${DCCL_LAST_REV} DCCL_LAST_REV)

   execute_process(COMMAND git rev-list ${DCCL_VERSION_MAJOR}.${DCCL_VERSION_MINOR}.${DCCL_VERSION_PATCH}..HEAD --count
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
   set(DCCL_VERSION_DATE "2019.02.05")
endif()


set(DCCL_VERSION "${DCCL_VERSION_MAJOR}.${DCCL_VERSION_MINOR}.${DCCL_VERSION_PATCH}")


include(today)
today(DCCL_COMPILE_DATE)

# give Dccl 2 series a few more soversions
set(DCCL_SOVERSION "30")

# fetch all the local directories for generated code
get_filename_component(dccl_SRC_DIR src ABSOLUTE)
set(dccl_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
get_filename_component(dccl_SCRIPTS_DIR scripts ABSOLUTE)

get_filename_component(dccl_LIB_DIR ${dccl_BUILD_DIR}/lib ABSOLUTE)
get_filename_component(dccl_BIN_DIR ${dccl_BUILD_DIR}/bin ABSOLUTE)
get_filename_component(dccl_INC_DIR ${dccl_BUILD_DIR}/include ABSOLUTE)
get_filename_component(dccl_SHARE_DIR ${dccl_BUILD_DIR}/share ABSOLUTE)

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

file(MAKE_DIRECTORY ${dccl_BIN_DIR})
install(DIRECTORY ${dccl_BIN_DIR}/ DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )

install(DIRECTORY ${dccl_SHARE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dccl)
install(DIRECTORY ${dccl_INC_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN  "*.h" PATTERN "*.proto" PATTERN "test*" EXCLUDE)

install(EXPORT dccl-config DESTINATION share/dccl/cmake)

message("Installation directories set to: \n\tlibraries: ${CMAKE_INSTALL_FULL_LIBDIR}\n\tbinaries: ${CMAKE_INSTALL_FULL_BINDIR}\n\theaders: ${CMAKE_INSTALL_FULL_INCLUDEDIR}")

## let cmake know where the headers are
include_directories(${dccl_INC_DIR})

## for MAC OS X 
if(${APPLE})
  ## MacPorts
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
  ## Fink
  include_directories(/sw/include)
  link_directories(/sw/lib)
endif()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  ${dccl_INC_DIR}/dccl
  ${dccl_FORMER_BIN_DIR}
  ${dccl_FORMER_LIB_DIR}
  ${dccl_FORMER_INC_DIR}
  ${dccl_FORMER_SHARE_DIR}
  python/dist
  )

## boost
find_package(Boost 1.40.0)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(ProtobufDCCL REQUIRED)
include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIRS})

message("Google Protocol Buffers Version: ${PROTOC_VERSION}")

# Protobuf uses pthread so we need to link it too (if it's here)
find_package(Threads)

# work around OLD compilers and systems (like gcc3.3 ...) where reentrancy matters to boost::signals2.  Dccl Bug #1090022.
add_definitions(-D_REENTRANT)
add_definitions(-pthread)

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
  include_directories(${Cryptopp_INCLUDE_DIRS})
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
  include_directories(${B64_INCLUDE_DIRS})
else()
  set(DCCL_HAS_B64 "0")
endif()

# Protobuf < 2.4.0 does not have plugin capability
if(${PROTOC_VERSION} VERSION_LESS 2.4.0)
  option(enable_units "Enable static unit-safety functionality" OFF)
else()
  option(enable_units "Enable static unit-safety functionality" ON)
endif()

# Protobuf >= 3.0.0. requires a syntax version
if(NOT ${PROTOC_VERSION} VERSION_LESS 3.0.0) 
  set(PROTOBUF_SYNTAX_VERSION "syntax = \"proto2\";")
endif()

# Protobuf >= 3.6.0 requires C++11 or greater
if((NOT ${PROTOC_VERSION} VERSION_LESS 3.6.0)
    AND ((NOT DEFINED CMAKE_CXX_STANDARD) OR ("${CMAKE_CXX_STANDARD}" STREQUAL "98")))
  message ("Setting CMAKE_CXX_STANDARD to 11 as required by Protobuf >= 3.6.0. If you wish to use a newer standard, set CMAKE_CXX_STANDARD directly")
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

## copy to dccl/include
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${dccl_SRC_DIR} src/*.h src/*.proto)
foreach(I ${INCLUDE_FILES})
  configure_file(${dccl_SRC_DIR}/${I} ${dccl_INC_DIR}/dccl/${I} @ONLY)
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dccl.h ${dccl_INC_DIR}/dccl.h @ONLY)

## copy to build/share
file(GLOB SHARE_FILES RELATIVE ${dccl_SRC_DIR} src/share/*)
foreach(I ${SHARE_FILES})
  file(COPY ${dccl_SRC_DIR}/${I} DESTINATION ${dccl_SHARE_DIR})
endforeach()

## start adding subdirectories
add_subdirectory(src)

