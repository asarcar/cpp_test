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

# Allows one to refer to header files in this directory 
include_directories(${CMAKE_CURRENT_SOURCE_DIR}) 

add_library(file_cache_lib file_cache.cc)

######################################
#           SubDirectories           #
######################################
if (CMAKE_CUSTOM_UNIT_TESTS)
  add_subdirectory(tests)
endif (CMAKE_CUSTOM_UNIT_TESTS)
