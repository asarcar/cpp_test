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
#include <chrono>    // std::chrono
#include <iostream>
#include <thread>
// Standard C Headers
#include <cstdlib>   // exit (EXIT_FAILURE)
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

// One does not need to include explicitly <gperftools/profiler.h> or
// call ProfilerStart/Stop to enable CPU profiling.
// The design of CPU profiling utility has been explicit in allowing
// it to profile applications and daemons never written with Google
// PerfTools in mind. Hence commenting out explicit code to test
// how the perftools work. 

// #include <gperftools/profiler.h>

void alloc_fn(int num_secs) {
  // ProfilerStart("test.cprof");
  std::cout << "FN " <<__FUNCTION__ << " LINE " << __LINE__ << std::endl;
  for (int i=0; i<num_secs; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Thread sleeping for 1 secs: iteration# " << i << std::endl;
    char *ch_ptr = new char[10];
    if (ch_ptr == NULL) {
      std::cerr << "Ran out of memory" << std::endl;
      return;
    }
    ch_ptr[0] = '\0';
    std::cout << "Allocated 10 bytes memory" << std::endl;
  }
  // ProfilerStop();
  return;
}

int main(int argc, char **argv) {
  int num_secs;
  if (argc != 2 || (num_secs = atoi(argv[1])) <= 0) {
    std::cerr << "Usage: " << argv[0] << " <num_secs>" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Entering " << argv[0] << " for " << num_secs 
            << " secs..." << std::endl;
  alloc_fn(num_secs);

  std::cout << "Exiting..." << std::endl;

  return 0;
}
