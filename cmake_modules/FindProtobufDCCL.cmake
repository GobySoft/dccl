# Locate and configure the Google Protocol Buffers library.
# Defers to the new CMake config shipped with Protobuf

# Adds the following functions:
# PROTOBUF_GENERATE_CPP_DCCL
# PROTOBUF_GENERATE_CPP_NO_DCCL
# PROTOBUF_GENERATE_CPP_LOAD_FILE


function(PROTOBUF_GENERATE_CPP_DCCL SRCS HDRS)
  if(enable_units)
    protobuf_generate_cpp_internal("True" "" PROTO_SRCS PROTO_HDRS ${ARGN})
  else()
    protobuf_generate_cpp_internal("False" "" PROTO_SRCS PROTO_HDRS ${ARGN})
  endif()
  set(${SRCS} ${PROTO_SRCS} PARENT_SCOPE)
  set(${HDRS} ${PROTO_HDRS} PARENT_SCOPE)
endfunction()

function(PROTOBUF_GENERATE_CPP_NO_DCCL SRCS HDRS)
  protobuf_generate_cpp_internal("False" "" PROTO_SRCS PROTO_HDRS ${ARGN})
  set(${SRCS} ${PROTO_SRCS} PARENT_SCOPE)
  set(${HDRS} ${PROTO_HDRS} PARENT_SCOPE)
endfunction()

function(PROTOBUF_GENERATE_CPP_LOAD_FILE LOAD_FILE SRCS HDRS)
  if(enable_units)
    protobuf_generate_cpp_internal("True" ${LOAD_FILE} PROTO_SRCS PROTO_HDRS ${ARGN})
  else()
    protobuf_generate_cpp_internal("False" "" PROTO_SRCS PROTO_HDRS ${ARGN})
  endif()
  set(${SRCS} ${PROTO_SRCS} PARENT_SCOPE)
  set(${HDRS} ${PROTO_HDRS} PARENT_SCOPE)
endfunction()


function(PROTOBUF_GENERATE_CPP_INTERNAL USE_DCCL LOAD_FILE SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    # /home/toby/dccl/src/core/proto/foo.proto
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    # foo
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    # core/proto/foo.proto
    file(RELATIVE_PATH REL_FIL ${dccl_SRC_DIR} ${ABS_FIL})
    # /home/toby/dccl/include/dccl/core/proto/foo.proto
    set(ABS_BUILT_FIL "${dccl_INC_DIR}/dccl/${REL_FIL}")
    # /home/toby/dccl/include/dccl/core/proto
    get_filename_component(FIL_PATH ${ABS_BUILT_FIL} PATH)

    # message(STATUS ${ABS_FIL})
    # message(STATUS ${FIL_WE})
    # message(STATUS ${REL_FIL})
    # message(STATUS ${FIL_PATH})

    include_directories(${FIL_PATH})

    list(APPEND ${SRCS} "${FIL_PATH}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${FIL_PATH}/${FIL_WE}.pb.h")

    if(USE_DCCL)
      string(COMPARE EQUAL "${LOAD_FILE}" "" result)
      if(result)
        set(DCCL_LOAD_FILE_ARG "")
        set(DCCL_PROTOC_COMMENT "Running C++ and DCCL protocol buffer compiler on ${FIL}")
      else()
        set(DCCL_LOAD_FILE_ARG "dccl3_load_file=${LOAD_FILE}:")
        set(DCCL_PROTOC_COMMENT "Running C++ and DCCL protocol buffer compiler on ${FIL}: load_file: ${LOAD_FILE}")
      endif()
      
      separate_arguments(DCCL_PROTOC_ARGS UNIX_COMMAND "--dccl_out=${DCCL_LOAD_FILE_ARG}${dccl_INC_DIR} --plugin ${dccl_EXEC_DIR}/protoc-gen-dccl")
    else()
      set(DCCL_PROTOC_COMMENT "Running C++ protocol buffer compiler on ${FIL}")
    endif()

    add_custom_command(
      OUTPUT "${FIL_PATH}/${FIL_WE}.pb.cc"
             "${FIL_PATH}/${FIL_WE}.pb.h" ${LOAD_FILE}
      COMMAND  ${Protobuf_PROTOC_EXECUTABLE}
      ARGS --cpp_out ${dccl_INC_DIR} --proto_path ${dccl_INC_DIR} ${dccl_INC_DIR}/dccl/${REL_FIL} -I ${Protobuf_INCLUDE_DIR} -I ${dccl_INC_DIR} ${DCCL_PROTOC_ARGS}
      # add guards for Clang static analyzer (scan-build)
      COMMAND /bin/bash
      ARGS -c "FILE=${FIL_PATH}/${FIL_WE}.pb.cc && TMPFILE=\${FILE}.\${RANDOM} && cat <(echo '#ifndef __clang_analyzer__') \${FILE} <(echo -e '\\n#endif // __clang_analyzer__') > \${TMPFILE} && mv \${TMPFILE} \${FILE}"
      DEPENDS ${ABS_FIL}
      COMMENT ${DCCL_PROTOC_COMMENT}
      VERBATIM )

  endforeach()

  # copy headers for generated headers
  file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${dccl_BUILD_DIR}/proto build/proto/*.h)
  foreach(I ${INCLUDE_FILES})
    configure_file(${dccl_BUILD_DIR}/proto/${I} ${dccl_INC_DIR}/${I} COPYONLY)
  endforeach()
  
  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

find_package(protobuf REQUIRED CONFIG)
find_package(absl REQUIRED CONFIG)
