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
pbuf_gen_cpp_fn(pbuf_src_files address_book.proto)
message("pbuf_src_files: ${pbuf_src_files}")
add_pbuf_ctest_fn("${pbuf_src_files}" abook_pbuf)

# add_executable(foo foo.cc ${PROTO_SRCS} ${PROTO_HDRS})
# target_link_libraries(foo ${PROTOBUF_LIBRARIES})
# > FIND_PACKAGE(Protobuf)
# > 
# > IF(PROTOBUF_FOUND)
# >
# >     INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
# >
# >     INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
# >
# >     PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS data.proto)
# >
# > ENDIF()

