// Copyright 2014 asarcar Inc.
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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <iostream>
#include <thread>         // std::thread::hardware_concurrency
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/init.h"
#include "utils/basic/proc_info.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class ProcInfoTester {
 public:
  ProcInfoTester() : proc_info_{ProcInfo::Singleton()} {}
  void Run(void) {
    CHECK_EQ(proc_info_->NumCores(), thread::hardware_concurrency());
    std::string flags = string("Flags=");
    for (auto entry : proc_info_->Flags()) {
      flags += string(" ") + entry;
    }
    LOG(INFO) << "Cores=" << proc_info_->NumCores() << ": " << flags;
    CHECK_EQ(proc_info_->CacheLineSize(), CACHE_LINE_SIZE);
  }
 private:
  ProcInfo *proc_info_;
};

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  LOG(INFO) << argv[0] << " Executing Test";
  ProcInfoTester pit;
  pit.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
