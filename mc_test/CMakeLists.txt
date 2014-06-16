# Author: Arijit Sarcar <asarcar@juniper.net>

# Centos: 2.6: cmake_minimum_required(VERSION 2.8)
cmake_minimum_required(VERSION 2.6)

project(MC_TEST)

# Centos GCC 4.4.7 doesn't have C++11: set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Wall")
set(CMAKE_CXX_FLAGS "-std=c++0x -ggdb -Wall")

message("Project MC_TEST: CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(BUILD_POSTFIX "_d")
else (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(BUILD_POSTFIX "")
endif (CMAKE_BUILD_TYPE STREQUAL Debug)


add_executable(mc_test mc_test.cc)

target_link_libraries(mc_test gflags glog memcached)

# Set the output directory for the binary
set_target_properties(mc_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(mc_test PROPERTIES OUTPUT_NAME mc_test${BUILD_POSTFIX})
