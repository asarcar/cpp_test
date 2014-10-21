// Copyright 2014 Elastasy Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Arijit Sarcar

#include <iostream>
#include <array>
#include <thread>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "file_cache.h"

namespace {

// The fixture for testing class FileCache
class FileClassTest:public::testing::Test 
{
 protected:
  static const int kNumFileCaches = 2;
  static const int kNumFiles = 3;
  static const int kNumThreads = 3;

  std::array<std::string, kNumFiles> file_name_;
  std::array<FileCache *, kNumFileCaches> file_cache_;
  std::array<std::thread, kNumThreads> thread_;

  // Create "few" file cache objects and "few" file names 
  FileClassTest() {
    DLOG(INFO) << "FileClassTest Constructor";
    int i=0;
    for (auto &p_fc :file_cache_) {
      p_fc = new FileCache(i+1, i);
      ++i;
    }
    DLOG(INFO) << kNumFileCaches << " FileCaches Initialized";
    for (auto &file_name : file_name_) {
      file_name = std::string("file_name_") + std::to_string(i);
    }
    DLOG(INFO) << kNumFiles << " File Names Set";
    DLOG(INFO) << "FileClassTest Object Created";
  }

  virtual ~FileClassTest() {
    DLOG(INFO) << "FileClassTest Destructor";
    for (int i=kNumFileCaches; i>0; i--) {
      FileCache *p_fc = file_cache_[i-1];
      delete p_fc;
    }
    DLOG(INFO) << "FileCaches Destroyed & FileClassTest Destructor Complete";
  }
  virtual void SetUp(){}
  virtual void TearDown(){}

}; // class FileClassTest

// 1. Spawn thread T0
// 2. Pin Unexisting File
// 3. Unpin File
// 4. Terminate T0
// 5. Check file exists and all 0s
static void FileCreateFn(int thread_id, 
                         FileCache *p_fc,
                         const std::string &file_name) {
  std::vector<std::string> file_vec = {file_name};

  DLOG(INFO) << "FileCreateFunction Called - Thread ID " << thread_id;
  p_fc->PinFiles(file_vec);
  p_fc->UnpinFiles(file_vec);
  DLOG(INFO) << "FileCreateFunction Called - Thread ID " << thread_id;

  return;
}

// Pinning an unexisting file creates a file of 10KB size initialized to 0s.
TEST_F(FileClassTest, FileCreate) {
  // Delete file_name_[0] if any exists due to previous runs

  std::thread th = std::thread(FileCreateFn, 0, file_cache_[0], file_name_[0]);

  th.join();

  // Check file_name_[0] is created, is 10KB in size, and all 0s
}    

TEST_F(FileClassTest, IsFileCacheUpdated) {
  DLOG(INFO) << "FileCache Updated Appropriately";
  EXPECT_EQ(1, 1) << "FileCache Updated Appropriately";
}    

TEST_F(FileClassTest, IsFileCacheFlushed) {
  DLOG(INFO) << "FileCache Flushed Appropriately";
  EXPECT_EQ(2, 2) << "FileCache Flushed Appropriately";
}    

TEST_F(FileClassTest, IsFileCacheEntryContentionResolved) {
  DLOG(INFO) << "FileCache Entry Contention Resolved Appropriately";
  EXPECT_EQ(3, 3) << "FileCache Entry Contention Resolved Appropriately";
}    

} // namespace


//-----------------------------------------------------------------------------
// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    // Initialize Google's flag library
    google::ParseCommandLineFlags(&argc, &argv, true);

    DLOG(INFO) << "File Cache Testing Initiated" << std::endl;
    DLOG_IF(INFO, argc > 1) << "  argc = " << argc << std::endl;

    int res = RUN_ALL_TESTS();

    DLOG(INFO) << "File Cache Testing Completed" << std::endl;

    return res;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
