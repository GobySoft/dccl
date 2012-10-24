# t. schneider tes@mit.edu 
# base CMakeLists.txt file for dccl
# for help on CMake, see http://www.cmake.org/cmake/help/documentation.html
# or type > man cmake

# this project is intended to be built out of source by using 
# > ./build.sh

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(dccl)

## allows us to write custom modules or modifying existing ones
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")


option(enable_testing "Enable building of tests using CTest (if set to ON, you can run tests with 'make test')" OFF)
if(enable_testing)
  enable_testing()
  include(CTest)
endif()

option(build_apps "Build supporting tools and other binary applications" ON)

set(DCCL_VERSION_MAJOR "3")
set(DCCL_VERSION_MINOR "0")
set(DCCL_VERSION_PATCH "0~alpha2")
set(DCCL_VERSION "${DCCL_VERSION_MAJOR}.${DCCL_VERSION_MINOR}.${DCCL_VERSION_PATCH}")
set(DCCL_VERSION_DATE "2012.08.24")

include(today)
today(DCCL_COMPILE_DATE)

# give Dccl 2 series a few more soversions
set(DCCL_SOVERSION "30")

# fetch all the local directories for generated code
get_filename_component(dccl_LIB_DIR lib ABSOLUTE)
get_filename_component(dccl_BIN_DIR bin ABSOLUTE)
get_filename_component(dccl_INC_DIR include ABSOLUTE)
get_filename_component(dccl_SRC_DIR src ABSOLUTE)
get_filename_component(dccl_SHARE_DIR share ABSOLUTE)
get_filename_component(dccl_BUILD_DIR build ABSOLUTE)
get_filename_component(dccl_SCRIPTS_DIR scripts ABSOLUTE)

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
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dccl_LIB_DIR} CACHE PATH 
  "Output directory for the dynamic libraries" )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dccl_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dccl_LIB_DIR} CACHE PATH 
  "Output directory for the static libraries (archives)" )

## set instructions for `make install`
file(MAKE_DIRECTORY ${dccl_BIN_DIR})
install(DIRECTORY ${dccl_BIN_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )

install(FILES ${dccl_SCRIPTS_DIR}/analyze_dccl_xml DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )

install(DIRECTORY ${dccl_LIB_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${dccl_SHARE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dccl)
install(DIRECTORY ${dccl_INC_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  FILES_MATCHING REGEX .*\\.h|.*\\.proto)

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

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${dccl_INC_DIR}/dccl)

## copy to dccl/include
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${dccl_SRC_DIR} src/*.h src/*.proto)
foreach(I ${INCLUDE_FILES})
  configure_file(${dccl_SRC_DIR}/${I} ${dccl_INC_DIR}/dccl/${I} @ONLY)
endforeach()

configure_file(${CMAKE_SOURCE_DIR}/dccl.h ${dccl_INC_DIR}/dccl.h @ONLY)


## boost
find_package(Boost 1.40.0 COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})

find_package(ProtobufDCCL REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# shared library suffix
add_definitions(-DSHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")

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
  add_definitions(-DHAS_CRYPTOPP)
  include_directories(${Cryptopp_INCLUDE_DIRS})
endif()

## start adding subdirectories
add_subdirectory(src)

