# Copyright 2014 Elastasy Inc.
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

# Author: Arijit Sarcar

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Link the executable to the file_cache library. 
add_executable(file_cache_test fc_test.cc)
target_link_libraries(file_cache_test file_cache_lib gflags glog ${GTEST_LIBRARIES} pthread)
# Register the test.
add_test(file_cache_test /bin/sh -c 
         "${CMAKE_CURRENT_BINARY_DIR}/file_cache_test --log_dir=\"${CMAKE_CURRENT_BINARY_DIR}\" --gtest_output=\"xml:${CMAKE_CURRENT_BINARY_DIR}/test_output.xml\"")
