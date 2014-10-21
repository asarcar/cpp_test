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
#include <array>
#include <iostream>
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

static const int kNumThreads = 5;

// Prints Hello world with ThreadId
void ThreadFn(int thread_id) {
  std::cout << "Hello from Thread-ID " << thread_id << std::endl;
}

int main(int argc, char ** argv) {
  std::array<std::thread, kNumThreads> th;
  
  std::cout << "Main: Going to spawn Threads" << std::endl;
  
  int i=0;
  for (auto &t :th) {
    t = std::thread(ThreadFn, i++);
  }

  std::cout << "Main: Spawned " << kNumThreads << " - joining back..." << std::endl;

  for (auto &t :th) {
    t.join();
  }

  std::cout << "Main: All threads Joined. Exiting." << std::endl;

  return 0;
}
