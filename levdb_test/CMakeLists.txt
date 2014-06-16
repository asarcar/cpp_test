cmake_minimum_required(VERSION 2.8)

project(LEVDB_TEST)

# Compiler and Flag Options
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -pthread")

# Find the Level DB Package
# find_package(leveldb REQUIRED)
# include_directories(${LEVELDB_INCLUDE_DIRECTORIES})

add_executable(levdb_write_test write_test.cc)
target_link_libraries(levdb_write_test leveldb snappy)

add_executable(levdb_read_test read_test.cc)
target_link_libraries(levdb_read_test leveldb snappy)
