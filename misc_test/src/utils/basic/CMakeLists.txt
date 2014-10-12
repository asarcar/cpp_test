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

add_library(basic_utils init.cc mem_block.cc proc_info.cc)
target_link_libraries(basic_utils gflags glog) 

# 1. heap-checker: Comes when you link with tcmalloc
# 2. cpu profiler: Comes when you link with profiler. 
# Note: Linking with tcmalloc or profiler does not 
# turn on heapcheck or CPU Profiling. It just inserts
# the code. As such, it's practical to just always link.
if (CMAKE_CUSTOM_DISABLE_GOOGLE_PERFTOOLS)
  # message("Google Profiler: Disabled")
else (CMAKE_CUSTOM_DISABLE_GOOGLE_PERFTOOLS)
  # message("Google Profiler: Enabled")
  target_link_libraries(basic_utils profiler)
  if (CMAKE_CUSTOM_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(basic_utils tcmalloc_debug)
  else (CMAKE_CUSTOM_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(basic_utils tcmalloc)
  endif (CMAKE_CUSTOM_BUILD_TYPE STREQUAL "Debug")
endif (CMAKE_CUSTOM_DISABLE_GOOGLE_PERFTOOLS)


######################################
#           SubDirectories           #
######################################
if (CMAKE_CUSTOM_UNIT_TESTS)
  add_subdirectory(tests)
endif (CMAKE_CUSTOM_UNIT_TESTS)
