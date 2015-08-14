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
#include <atomic>           // std::atomic
#include <chrono>           // std::chrono
#include <iostream>         // std::cout
#include <random>           // std::distribution, random engine, ...
#include <thread>           // std::thread
#include <vector>           // std::vector
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/spin_lock.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);
DECLARE_bool(benchmark);

class SpinLockTester {
 public:
  SpinLockTester(): sl_{} {}
  ~SpinLockTester() = default;

  void LockBasicTest();
  void LockExclusiveTest();
  void LockSharedTest();
  void LockBothTest();
  void LockUpgradeTest();
  void LockDowngradeTest();
  void LockBenchmarkTest();

 private:
  SpinLock sl_;
};


// Main thread holds the lock for 10ms. 
// Child Thread tries to hold the lock.
// Verify it does not succeed until main thread
// relinquishes the lock.
void SpinLockTester::LockBasicTest() {
  using UTime = chrono::microseconds;
  constexpr const char* unit = "us";
  constexpr uint32_t kSleepDuration = 10000;
  chrono::time_point<chrono::system_clock> 
      lock_start = chrono::system_clock::now();
  chrono::time_point<chrono::system_clock> 
      lock_acquire{lock_start};


  sl_.lock();
  thread th = thread(
      [this, &lock_start, &lock_acquire]() {
        CHECK(!sl_.TryLock());
        lock_acquire = chrono::system_clock::now();
        UTime duration = 
        chrono::duration_cast<UTime>(lock_acquire - lock_start);
        
        LOG(INFO) << "Child Thread: could not acquire SpinLock: sl " 
        << sl_ << ": even after " << duration.count() << unit; 
        sl_.lock();
        lock_acquire = chrono::system_clock::now();
        sl_.unlock();
      });
  // sleep for some time
  this_thread::sleep_for(UTime(kSleepDuration)); 
  sl_.unlock();

  th.join();
  chrono::nanoseconds duration = 
      chrono::duration_cast<chrono::nanoseconds>(lock_acquire - lock_start);
  CHECK_GT(duration.count(), kSleepDuration);
  LOG(INFO) << "Child Thread: acquired SpinLock: sl " << sl_ << " in "
            << duration.count() << unit; 

  return;
}

void SpinLockTester::LockExclusiveTest() {
  return;
}

void SpinLockTester::LockSharedTest() {
  return;
}

void SpinLockTester::LockBothTest() {
  return;
}

void SpinLockTester::LockUpgradeTest() {
  return;
}

void SpinLockTester::LockDowngradeTest() {
  return;
}

void SpinLockTester::LockBenchmarkTest() {
  return;
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  SpinLockTester test{};
  test.LockBasicTest();
  test.LockExclusiveTest();
  test.LockSharedTest();
  test.LockBothTest();
  test.LockUpgradeTest();
  test.LockDowngradeTest();
  if (FLAGS_benchmark)
    test.LockBenchmarkTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_bool(benchmark, false, 
            "test run when benchmarking spinlock Lock/Unlock"); 
