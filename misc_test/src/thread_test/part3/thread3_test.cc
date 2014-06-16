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
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
// Standard C Headers
#include <cstdlib>
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

static const std::size_t kNumThreads = 5;
static const std::size_t kArraySize = 5*1000;
static const int kMaxWaitSecs = 10;
static const int kVal1 = 1;
static const int kVal2 = 2;

std::mutex g_mutex;

typedef std::array<std::thread, kNumThreads> threadArray;
typedef std::array<int, kNumThreads+1> rangeArray;
typedef std::array<int, kArraySize> intArray;

// Prints Hello world with ThreadId
void VectorSum(const int thread_id, 
               const intArray &arr1, const intArray &arr2, 
               int *result_p, 
               const int low, const int high) {
  // Thread computes the sum of elements of arrays: index low to high
  int tmp_result = 0;  
  for (int i=low; i<high; i++) {
    tmp_result += arr1[i] + arr2[i];
  }
  
  // Simulate variable time required by threads to compute
  std::this_thread::sleep_for(std::chrono::seconds(rand() % kMaxWaitSecs));
  
  g_mutex.lock();
  *result_p += tmp_result;
  g_mutex.unlock();

  std::cout << "Part Result of Thread-ID " << thread_id << " tmp_result " << tmp_result << std::endl;
  std::cout << "Result Computed: Thread-ID " << thread_id << " result " << *result_p << std::endl;

  return;
}

int main(int argc, char ** argv) {
  threadArray th;
  intArray arr1;
  intArray arr2;

  // init arr1 with val1, arr2 with val2
  for (auto &v : arr1)
    v = kVal1;
  for (auto &v : arr2)
    v = kVal2;

  // Split array into kNumThreads parts
  rangeArray limit;
  int j=0;
  for (auto &lim_val : limit) {
    lim_val = (j*kArraySize)/kNumThreads;
    j++;
  }
  
  std::cout << "Main: Going to spawn " << kNumThreads << " Threads" <<
      " On 2 arrays of size " << kArraySize << " with Val " << kVal1 << 
      " and " << kVal2 << std::endl;
  
  int result=0;
  int i=0;
  for (auto &t :th) {
    std::cout << "Thread " << i << " called on arr1 & arr2: limit[" << 
        i << "]=" << limit[i] << " limit[" << i+1 << "]=" << limit[i+1] << std::endl; 
    t = std::thread(VectorSum, i, arr1, arr2, &result, limit[i], limit[i+1]);
    i++;
  }

  std::cout << "Main: Spawned " << kNumThreads << " - joining back..." << std::endl;

  for (auto &t :th) {
    t.join();
  }

  std::cout << "Main: All threads Joined: Sum: " << result << std::endl;

  return 0;
}
