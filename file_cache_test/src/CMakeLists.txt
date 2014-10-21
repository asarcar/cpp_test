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
cmake_minimum_required(VERSION 2.8)

project(FILECACHE)

get_filename_component(FILECACHE_TOP_DIR "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)

# Compiler and Flag Options
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++0x -ggdb -Wall -pthread")

enable_testing()

# Set the compiler's include paths: 
# All includes are relative to base of source directories OR
include_directories(${FILECACHE_SOURCE_DIR})

add_subdirectory(fc)
add_subdirectory(test)
