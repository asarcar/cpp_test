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
#include "utils/concur/lock_guard.h"
#include "utils/concur/spin_lock.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;
using namespace std::chrono;

// Declarations
DECLARE_bool(auto_test);
DECLARE_bool(benchmark);

class SpinLockTester {
 public:
  SpinLockTester() {}
  ~SpinLockTester() = default;

  void LockBasicTest();
  void LockExclusiveTest();
  void LockSharedTest();
  void LockBothTest();
  void LockUpgradeTest();
  void LockDowngradeTest();
  void LockBenchmarkTest();

 private:
  using UTime = microseconds;
  using TimeP = time_point<system_clock>;
  static constexpr const char* kUnitStr = "us";
  static constexpr uint32_t kSleepDuration = 10000;

  SpinLock   sl_;
  struct LockFields {
    SpinLock& sl;
    TimeP     start;
    UTime     duration;
    bool      try_lock_status;
    LockMode  parent_lock_mode;
    LockMode  child_lock_mode;
  };
  void LockHelper(LockFields& lf);
};

constexpr const char* SpinLockTester::kUnitStr;
constexpr uint32_t SpinLockTester::kSleepDuration;

void 
SpinLockTester::LockHelper(LockFields& lf) {
  thread th;  
  {
    lock_guard<SpinLock> lckg(lf.sl, lf.parent_lock_mode);
    th = thread(
        [&lf]() {
          lf.try_lock_status = lf.sl.TryLock(lf.child_lock_mode);
          if (lf.try_lock_status)
            lf.sl.unlock();
          {
            lock_guard<SpinLock> lckg(lf.sl, lf.child_lock_mode);
            lf.duration = 
                duration_cast<UTime>(system_clock::now() - lf.start);
            LOG(INFO) << "Child Thread " << this_thread::get_id()
                      << ": Parent Held Lock " 
                      << Lock::to_string(lf.parent_lock_mode)
                      << ": Child Trying Lock " 
                      << Lock::to_string(lf.child_lock_mode)
                      << ": TryLockStatus " << boolalpha << lf.try_lock_status
                      << ": Lock Acquired " << lf.sl << " in "
                      << lf.duration.count() << kUnitStr; 
          }
        });
    // sleep for some time
    this_thread::sleep_for(UTime(kSleepDuration)); 
  }
  th.join();

  LOG(INFO) << "Parent Thread " << this_thread::get_id()
            << ": Parent Held Lock " 
            << Lock::to_string(lf.parent_lock_mode)
            << ": Child Lock " 
            << Lock::to_string(lf.child_lock_mode)
            << ": TryLockStatus " << boolalpha << lf.try_lock_status
            << ": Lock Acquired " << lf.sl << " in "
            << lf.duration.count() << kUnitStr; 
  return;
}

// 1. TryLock acquire succeeds on first attempt
// 2. TryLock fails if lock already acquired
void SpinLockTester::LockBasicTest() {
  CHECK(sl_.TryLock());
  sl_.unlock();
  lock_guard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK); 
  CHECK(!sl_.TryLock());

  return;
}

// Main thread holds the lock for 10ms. 
// Child Thread tries to hold the lock.
// Verify it does not succeed until main thread
// relinquishes the lock.
void SpinLockTester::LockExclusiveTest() {
  LockFields lf = {sl_, system_clock::now(), {}, false, 
                   LockMode::EXCLUSIVE_LOCK, LockMode::EXCLUSIVE_LOCK};
  LockHelper(lf);
  CHECK(!lf.try_lock_status);
  CHECK_GT(lf.duration.count(), kSleepDuration);
  return;
}

// 1. Parent holds shared lock. Child allowed Shared lock.
// 2. Parent holds shared Lock. Child disallowed Exclusive 
//    Lock until parent relinquishes shared lock.
// 3. Parent holds exclusive Lock. Child disallowed shared
//    Lock until parent relinquishes exclusive lock.
void SpinLockTester::LockSharedTest() {
  {
    LockFields lf = {sl_, system_clock::now(), {}, false, 
                     LockMode::SHARE_LOCK, LockMode::SHARE_LOCK};
    LockHelper(lf);
    CHECK(lf.try_lock_status);
    CHECK_LT(lf.duration.count(), kSleepDuration);
  }
  {
    LockFields lf = {sl_, system_clock::now(), {}, false, 
                     LockMode::SHARE_LOCK, LockMode::EXCLUSIVE_LOCK};
    LockHelper(lf);
    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration.count(), kSleepDuration);
  }
  {
    LockFields lf = {sl_, system_clock::now(), {}, false, 
                     LockMode::EXCLUSIVE_LOCK, LockMode::SHARE_LOCK};
    LockHelper(lf);
    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration.count(), kSleepDuration);
  }
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
