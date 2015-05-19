# Copyright 2014 asarcar Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Author: Arijit Sarcar <sarcar_a@yahoo.com>

cmake_minimum_required(VERSION 2.8)

project (CPP_MISC_TEST)

###################################################
#         Global Debug Settings                   #
###################################################
# message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
# message("CMAKE_VERBOSE_MAKEFILE is ${CMAKE_VERBOSE_MAKEFILE}")
# message("CMAKE_CUSTOM_BUILD_DOCUMENTATION is ${CMAKE_CUSTOM_BUILD_DOCUMENTATION}")
# message("CMAKE_CUSTOM_DASHBOARD_SCRIPTING is ${CMAKE_CUSTOM_DASHBOARD_SCRIPTING}")
# message("CMAKE_CUSTOM_UNIT_TESTS is ${CMAKE_CUSTOM_UNIT_TESTS}")

#######################################################
#                Directory Settings                   #
#######################################################
# message("CMAKE_SOURCE_DIR is ${CMAKE_SOURCE_DIR}")
# message("CMAKE_BUILD_DIR is ${CMAKE_BUILD_DIR}")
# message("CMAKE_TEST_OP_DIR is ${CMAKE_TEST_OP_DIR}")
# message("CMAKE_BIN_DIR is ${CMAKE_BIN_DIR}")
# message("CMAKE_DOC_DIR is ${CMAKE_DOC_DIR}")

###############################################
#                CMAKE settings               #
###############################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

###############################################
#                Build settings               #
###############################################
#  (1) Debug: 
#        Includes debugging symbols, does NOT produce optimized
#        code, includes debugging assertions. 
#        Suitable for DEV, UT, and system functionality testing.
#  (2) Release: 
#        Includes debugging symbols, produces optimized code, does NOT 
#        include debugging assertions. 
#        Suitable for system performance, benchmarking, and 
#        release ready production binaries for customers.
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") 
  # LEVEL: All assert LEVELS are reacted (even including WARNINGs)
  # MODE: LOG and throw exception for reacted assert failure
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFASSERT_REACTION_LEVEL=2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFASSERT_REACTION_MODE=1")
  set(BUILD_POSTFIX "_d")
else (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -DNDEBUG")
  # LEVEL: Only react to FATAL asserts
  # MODE: LOG and Terminate for reacted assert failure
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFASSERT_REACTION_LEVEL=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFASSERT_REACTION_MODE=2")
  set(BUILD_POSTFIX "")
endif (CMAKE_BUILD_TYPE STREQUAL Debug)

###############################################
#                Build settings               #
###############################################
if (CMAKE_CUSTOM_DISABLE_GOOGLE_PERFTOOLS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_GOOGLE_PERFTOOLS=1") 
endif (CMAKE_CUSTOM_DISABLE_GOOGLE_PERFTOOLS)

######################################################
#         C/C++ Complier related settings            #
######################################################
# ccache.samba.org: 
#   ccache is a compiler cache. It speeds up recompilation 
#   by caching previous compilations and detecting when the same 
#   compilation is being done again. Only works with GNU.
set(CMAKE_CXX_COMPILER "ccache")
set(CMAKE_CXX_COMPILER_ARG1 "g++")
# code.google.com/p/distcc: 
#   when compilation times become a problem we can look
#   at distcc: by replacing ccache with distcc.

# Set common compiler flags:
# 1. -Werror: to treat all warnings as errors and fail compilation
# 2. -Wall: validate all kinds of checks and give warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

# 4. -D__STDC_FORMAT_MACROS: allows one to use format macros PRIu64
#     e.g. printf("test uint64_t : %" PRIu64 "\n", ui64);
# 5. -D_GNU_SOURCE: Enable all non-standard GNU Extensions
#    5.1. access to traditional functions which were omitted from 
#         the POSIX standard (often for good reason, such as being 
#         replaced with better alternatives, or being tied to particular 
#         legacy implementations)
#    5.2. access to low-level functions that cannot be portable, but 
#         that you sometimes need for implementing system utilities 
#         like mount, ifconfig, etc.
#    5.3. broken behavior for lots of POSIX-specified functions, where 
#         the GNU folks disagreed with the standards committee on how 
#         the functions should behave and decided to do their own thing.
#    5.4. enables a whole lot of compiling flags: STRICT_ANSI, 
#         _ISOC99_SOURCE, _POSIX_SOURCE, _POSIX_C_SOURCE, _XOPEN_SOURCE, 
#         _XOPEN_SOURCE_EXTENDED, _LARGEFILE_SOURCE, _LARGEFILE64_SOURCE, 
#         _FILE_OFFSET_BITS=N, _BSD_SOURCE, _SVID_SOURCE
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS -D_GNU_SOURCE")

# 6. -fno-omit-frame-pointer: Keep the frame pointer for all functions.
#    The omit-frame-pointer is enabled at several optimization levels.
#    The intent was to avoid the instructions to save, set up and restore 
#    frame pointers; it also made an extra register available in many fns. 
#    It made debugging impossible on some machines.
# 7. -funsigned-char: let the type "char" be interpreted as "unsigned" entity
#    Each kind of machine has a default for what char should be. It is either 
#    like unsigned char by default or like signed char by default. Since 
#    linux based SW and most open source code (e.g. from google) were written 
#    with unsigned char in mind, we stick to that option. 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -funsigned-char")

# 8. -mcx16: Enables GCC to use CMPXCHG16B instruction in generated code. 
#    CMPXCHG16B allows for atomic operations on 128-bit double quadword 
#    (or oword) data types. 
#    This is useful for high resolution counters that could be updated by 
#    multiple processors (or cores). This instruction is generated as part 
#    of atomic built-in functions.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")

# 9. -fno-builtin-malloc -fno-builtin-calloc 
#    -fno-builtin-realloc -fno-builtin-free
#    While strictly not required when using tcmalloc, these options are 
#    for the paranoid to ensure that all memory management routines
#    never refer to the one provided by gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-malloc") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-calloc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-realloc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-free")

# 10. Ubuntu 13.10 gcc 4.8.1 has a bug: use compiler options -Wl,--no-as-needed 
#     to link with threads (-pthread) or profile (-lprofile): profiling would
#     be tested in prof_test use cases
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -pthread")

# Finally we set the C++11 compiler option
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Set the compiler's include paths: 
# All includes are relative to base of source directories
include_directories(${CMAKE_SOURCE_DIR}) 
##########################################################
# FIND Packages, Include Directories, Set/Target Library #
##########################################################
# Protobuf package.
find_package(Protobuf REQUIRED)
# For Protobuf or other generated files from top of build directory
include_directories(${CMAKE_BUILD_DIR})

# Zero MQ
find_package(ZeroMQ REQUIRED)
include_directories(${ZeroMQ_INCLUDE_DIR})

#######################################
# Enable Dashboard Scripting or CTest #
#######################################
if (CMAKE_CUSTOM_DASHBOARD_SCRIPTING)
  include (CTest)   
  # message("CDash and Test Enabled")
else (CMAKE_CUSTOM_DASHBOARD_SCRIPTING)
  if (CMAKE_CUSTOM_UNIT_TESTS)
    enable_testing()
    # message("CTest Enabled")
  endif (CMAKE_CUSTOM_UNIT_TESTS)
endif (CMAKE_CUSTOM_DASHBOARD_SCRIPTING)

##############################
#          Functions         #
##############################
function(register_test test_name)
  # Test Dump Director: same relative path wrt to the base directory.
  file(RELATIVE_PATH rel_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  set(test_op_dir ${CMAKE_TEST_OP_DIR}/${rel_path}/${test_name})

  set(test_command "/bin/mkdir -p ${test_op_dir} && ")
  set(test_command 
      "${test_command} ${CMAKE_CURRENT_BINARY_DIR}/${test_name}${BUILD_POSTFIX}")
  set(test_command "${test_command} --auto_test=true")    

  # We support optional arguments to this function - they'll all be passed on
  # to the test.
  foreach(optional_arg ${ARGN})
    set(test_command "${test_command} ${optional_arg}")
  endforeach(optional_arg)

  # Run the test.
  add_test(${test_name} /bin/sh -c "${test_command}")

  # Set the test timeout and environment.
  set(test_env "SOURCE_DIR=${CMAKE_SOURCE_DIR}")
  set(test_env "${test_env};HEAPCHECK_DUMP_DIRECTORY=${test_op_dir}")
  set(test_env "${test_env};PPROF_PATH=/usr/bin/google-pprof")

  # HeapProfiler and CPUProfiler: Not enabled in tests: however the executables
  # are profile ready as they are linked using -lprofiler and -ltcmalloc
  # Beware of a linker issue in g++ that may require to use LD_PRELOAD to 
  # ensure CPU profiler works.
  # > env LD_PRELOAD=/path/to/libprofiler.so env CPUPROFILE=myctest.cprof ./myctest ...
  set(heap_prof_name "${test_op_dir}/${test_name}${BUILD_POSTFIX}.hprof")
  set(test_env "${test_env};HEAPPROFILE=${heap_prof_name}")
  set(cpu_prof_name "${test_op_dir}/${test_name}${BUILD_POSTFIX}.cprof")
  set(test_env "${test_env};CPUPROFILE=${cpu_prof_name}")

  set(test_env "${test_env};TEST_OUTPUT_DIR=${test_op_dir}")
  # message("<test_env test_command> is <${test_env} ${test_command}>")
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 45
                       ENVIRONMENT "${test_env}")
  set_property(TARGET ${test_name} 
               PROPERTY OUTPUT_NAME "${test_name}${BUILD_POSTFIX}")

endfunction(register_test)

function(add_test_fn test_name)
  set(tmpname ${test_name}_test)
  add_executable(${tmpname} ${tmpname}.cc)
  target_link_libraries(${tmpname} basic_utils)
  # All optional arguments are libraries 
  foreach(optional_arg ${ARGN})
    target_link_libraries(${tmpname} ${optional_arg})
  endforeach(optional_arg)
  set_target_properties(${tmpname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR})
  set_target_properties(${tmpname} PROPERTIES OUTPUT_NAME ${tmpname}${BUILD_POSTFIX})
endfunction(add_test_fn)

function(add_ctest_fn test_name)
  set(tmpname ${test_name}_test)
  add_executable(${tmpname} ${tmpname}.cc)
  target_link_libraries(${tmpname} basic_utils)
  # All optional arguments are libraries 
  foreach(optional_arg ${ARGN})
    target_link_libraries(${tmpname} ${optional_arg})
  endforeach(optional_arg)
  register_test(${tmpname})
endfunction(add_ctest_fn)

# protobuf_generate_cpp() provided by CMAKE 
# 1) Does not allow tweaking --proto_path & --cpp_out args to protoc
# 2) Is such that sets GENERATED flags true for PROTO_SRC and PROTO_HEADER.
# 3) Requires add_executable/library() calls within the same directory.
# Below is a custom function to work around these limitations.

function(pbuf_gen_cpp_fn pbuf_src_files)
  if(NOT ARGN)
    message(FATAL_ERROR "pbuf_gen_cpp_fn called without any proto files")
    return()
  endif(NOT ARGN)

  # --proto_path arguments to protoc includes all files referred
  # from project source directory and implicit in 
  # PROTOBUF_INCLUDE_DIR - The include directory for protocol buffers
  set(pbuf_proto_args "--proto_path=${CMAKE_SOURCE_DIR}")
  foreach(pbuf_inc_dir ${PROTOBUF_INCLUDE_DIRS})
    list(APPEND pbuf_proto_args "--proto_path=${pbuf_inc_dir}")
  endforeach(pbuf_inc_dir)

  set(pbuf_src_files) 
  foreach(pbuf_file ${ARGN})
    # ABSOLUTE  = Full path to file
    get_filename_component(pbuf_abs_path ${pbuf_file} ABSOLUTE)
    # NAME_WE = File name without directory or longest extension
    get_filename_component(pbuf_base ${pbuf_file} NAME_WE)
    set(pbuf_srcs
        "${CMAKE_CURRENT_BINARY_DIR}/${pbuf_base}.pb.h"
        "${CMAKE_CURRENT_BINARY_DIR}/${pbuf_base}.pb.cc")
    list(APPEND pbuf_src_files ${pbuf_srcs})
    # PROTOBUF_PROTOC_EXECUTABLE - The protoc compiler
    add_custom_command(
      OUTPUT ${pbuf_srcs}
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS ${pbuf_proto_args} --cpp_out=${CMAKE_BUILD_DIR}  ${pbuf_abs_path}
      DEPENDS ${pbuf_abs_path}
      COMMENT "Running protoc on ${pbuf_file}"
      VERBATIM
    )
  endforeach(pbuf_file)
  set_source_files_properties(${pbuf_src_files} PROPERTIES GENERATED TRUE)
  set(pbuf_src_files ${pbuf_src_files} PARENT_SCOPE)
endfunction(pbuf_gen_cpp_fn)

function(add_pbuf_ctest_fn pbuf_src_files test_name)
  set(tmpname ${test_name}_test)
  message("pbuf_src_files is ${pbuf_src_files}")
  add_executable(${tmpname} ${tmpname}.cc ${pbuf_src_files})
  target_link_libraries(${tmpname} basic_utils ${PROTOBUF_LIBRARIES})
  # All optional arguments are libraries 
  foreach(optional_arg ${ARGN})
    target_link_libraries(${tmpname} ${optional_arg})
  endforeach(optional_arg)
  register_test(${tmpname})
endfunction(add_pbuf_ctest_fn)

######################################
#           SubDirectories           #
######################################
add_subdirectory(experiment_test)
add_subdirectory(utils)
# Generate documentation for all doxygen annotated files
add_subdirectory(doc_framework)