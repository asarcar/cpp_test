// Copyright 2015 asarcar Inc.
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
#include "utils/basic/clock.h"
#include "utils/basic/init.h"
#include "utils/concur/lock_guard.h"
#include "utils/concur/rw_mutex.h"
#include "utils/concur/spin_lock.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;

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
  void LockUpgradeTest();
  void LockDowngradeTest();
  void LockBenchmarkTest();

 private:
  static constexpr const char* kUnitStr = "us";
  static constexpr uint32_t kSleepDuration = 10000;
  static constexpr uint32_t kNumLockUnlock = 10000;

  SpinLock   sl_{};
  std::mutex m_{};   // used only when benchmarking against spinlock
  RWMutex    rwm_{}; // used only when benchmarking against spinlock

  struct LockFields {
    SpinLock&            sl;
    Clock::TimePoint     start;
    Clock::TimeDuration  duration;
    bool                 try_lock_status;
    LockMode             child_lock_mode;
  };
  static void LockHelper(LockFields& lf);
  template <typename Lock>
  Clock::TimeDuration LockBenchmarkExclusiveHelper(Lock &m);
  template <typename Lock>
  Clock::TimeDuration LockBenchmarkShareHelper(Lock &m);
};

constexpr const char* SpinLockTester::kUnitStr;
constexpr uint32_t SpinLockTester::kSleepDuration;
constexpr uint32_t SpinLockTester::kNumLockUnlock;

void 
SpinLockTester::LockHelper(LockFields& lf) {
  LOG(INFO) << "Child Thread " << std::this_thread::get_id()
            << ": Parent Held Lock " 
            << Lock::to_string(lf.sl.Mode())
            << ": Child Trying Lock " 
            << Lock::to_string(lf.child_lock_mode);

  lf.try_lock_status = lf.sl.TryLock(lf.child_lock_mode);
  if (lf.try_lock_status)
    lf.sl.unlock();
  {
    LockGuard<SpinLock> lckg(lf.sl, lf.child_lock_mode);
    lf.duration = Clock::USecs() - lf.start;
    LOG(INFO) << "Child Thread " << std::this_thread::get_id()
              << ": TryLockStatus " << std::boolalpha << lf.try_lock_status
              << ": Lock Acquired " << lf.sl << " in "
              << lf.duration << kUnitStr; 
  }
  return;
}

// 1. TryLock acquire succeeds on first attempt
// 2. TryLock fails if lock already acquired
void SpinLockTester::LockBasicTest() {
  {
    CHECK(sl_.TryLock());
    CHECK_EQ(sl_.Mode(), LockMode::EXCLUSIVE_LOCK);
    sl_.unlock();
    CHECK_EQ(sl_.Mode(), LockMode::UNLOCK);
  }
  {
    LockGuard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK); 
    CHECK(!sl_.TryLock());
    CHECK_EQ(sl_.Mode(), LockMode::EXCLUSIVE_LOCK);
  }
  CHECK_EQ(sl_.Mode(), LockMode::UNLOCK);

  LOG(INFO) << __FUNCTION__ << " passed";
  return;
}

// Main thread holds the lock for 10ms. 
// Child Thread tries to hold the lock.
// Verify it does not succeed until main thread
// relinquishes the lock.
void SpinLockTester::LockExclusiveTest() {
  std::thread th;
  LockFields lf = {sl_, Clock::USecs(), 0, false, 
                   LockMode::EXCLUSIVE_LOCK};
  {
    LockGuard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK);
    th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
    // sleep for some time
    std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
  }
  th.join();

  CHECK(!lf.try_lock_status);
  CHECK_GT(lf.duration, kSleepDuration);

  LOG(INFO) << __FUNCTION__ << " passed";
  return;
}

// 1. Parent holds shared lock. Child allowed Shared lock.
// 2. Parent holds shared Lock. Child disallowed Exclusive 
//    Lock until parent relinquishes shared lock.
// 3. Parent holds exclusive Lock. Child disallowed shared
//    Lock until parent relinquishes exclusive lock.
void SpinLockTester::LockSharedTest() {
  std::thread th;
  {
    LockFields lf = {sl_, Clock::USecs(), 0, false, 
                     LockMode::SHARE_LOCK};
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
      th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
      // sleep for some time
      std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
      CHECK(lf.try_lock_status);
      CHECK_LT(lf.duration, kSleepDuration);
    }
    th.join(); 
  }
  {
    LockFields lf = {sl_, Clock::USecs(), 0, false, 
                     LockMode::EXCLUSIVE_LOCK};
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
      th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
      // sleep for some time
      std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
    }
    th.join();

    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration, kSleepDuration);
  }
  {
    LockFields lf = {sl_, Clock::USecs(), 0, false, 
                     LockMode::SHARE_LOCK};
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK);
      th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
      // sleep for some time
      std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
    }
    th.join();

    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration, kSleepDuration);
  }

  LOG(INFO) << __FUNCTION__ << " passed";
  return;
}

// 1. Take SHARED_LOCK. TryLock EXCLUSIVE_LOCK fails. 
//    UpgradeLock succeeds. Subsequent TryUpgrade fails.
// 2. Take SHARED LOCK twice. TryUpgrade fails.
//    Unlock SHARED LOCK one. TryUpgrade succeeds.
// 3. Parent takes SHARED LOCK. CHILD thread Lock succeeds. 
//    Parent upgrades lock. Next Child thread lock acquire 
//    fails until parent releases lock
void SpinLockTester::LockUpgradeTest() {
  {
    LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
    CHECK_EQ(sl_.Mode(), LockMode::SHARE_LOCK);
    CHECK(!sl_.TryLock(LockMode::EXCLUSIVE_LOCK));
    CHECK(sl_.TryUpgrade());
    CHECK_EQ(sl_.Mode(), LockMode::EXCLUSIVE_LOCK);
  }
  {
    LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
      CHECK(!sl_.TryUpgrade());
    }
    CHECK(sl_.TryUpgrade());
  }

  std::thread th;
  {
    LockFields lf = {sl_, Clock::USecs(), 0, false, 
                     LockMode::SHARE_LOCK};
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::SHARE_LOCK);
      CHECK_EQ(sl_.Mode(), LockMode::SHARE_LOCK);
      CHECK(sl_.TryUpgrade());
      CHECK_EQ(sl_.Mode(), LockMode::EXCLUSIVE_LOCK);
      th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
      // sleep for some time
      std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
    }
    th.join();

    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration, kSleepDuration);
  }

  LOG(INFO) << __FUNCTION__ << " passed";
  return;
}

// 1. Take EXCLUSIVE_LOCK. Downgrade. Verify mode is SHARE_LOCK. 
// 2. Parent takes EXCLUSIVE LOCK. CHILD thread tries SHARE_LOCK.
//    Parent Downgrades lock. CHILD Lock succeeds.
void SpinLockTester::LockDowngradeTest() {
  {
    LockGuard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK);
    sl_.Downgrade();
    CHECK_EQ(sl_.Mode(), LockMode::SHARE_LOCK);
  }
  LOG(INFO) << __FUNCTION__ << " basic downgrade exclusive->shared passed";

  std::thread th;
  {
    LockFields lf = {sl_, Clock::USecs(), 0, false, 
                     LockMode::SHARE_LOCK};
    {
      LockGuard<SpinLock> lckg(sl_, LockMode::EXCLUSIVE_LOCK);
      th = std::thread(SpinLockTester::LockHelper, std::ref(lf));
      // sleep for some time
      std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
      sl_.Downgrade();
      CHECK_EQ(sl_.Mode(), LockMode::SHARE_LOCK);
      th.join();
    }
    CHECK(!lf.try_lock_status);
    CHECK_GT(lf.duration, kSleepDuration);
  }

  LOG(INFO) << __FUNCTION__ << " passed";
  return;
}

template <typename Lock>
Clock::TimeDuration SpinLockTester::LockBenchmarkExclusiveHelper(Lock &m) {
  auto fn = [&m]() {
    for (int i=0; i<kNumLockUnlock; ++i) {
      LockGuard<Lock> lckg(m);
    }
  };

  Clock::TimePoint now = Clock::USecs();
  std::thread th = std::thread(fn);
  fn();
  th.join();

  return (Clock::USecs() - now);
}

template <typename Lock>
Clock::TimeDuration SpinLockTester::LockBenchmarkShareHelper(Lock &m) {
  auto fn = [&m]() {
    for (int i=0; i<kNumLockUnlock; ++i) {
      LockGuard<Lock> lckg(m, LockMode::SHARE_LOCK);
    }
  };

  Clock::TimePoint now = Clock::USecs();
  std::thread th = std::thread(fn);
  fn();
  th.join();

  return (Clock::USecs() - now);
}

// 1. Spawn 2 threads (Parent/Child) and repeatedly try to
//    grab and release SpinLock in EXCLUSIVE_LOCK mode. 
//    Execute the same test using mutex
//    
// 2. Spawn 2 threads (Parent/Child) and repeatedly try to
//    grab and release SpinLock in SHARE_LOCK mode.
//    Execute the same test using mutex
void SpinLockTester::LockBenchmarkTest() {
  Clock::TimeDuration durM = 
      LockBenchmarkExclusiveHelper(m_);
  Clock::TimeDuration durS = 
      LockBenchmarkExclusiveHelper(sl_);
  LOG(INFO) << "Time: " << kNumLockUnlock << " lock/unlock pairs "
            << "for Mutex/SpinLock = " << durM << "/" << durS 
            << kUnitStr << ": SpeedUp = " 
            << static_cast<double>(durM)/static_cast<double>(durS);

  durM = LockBenchmarkShareHelper(rwm_);
  durS = LockBenchmarkShareHelper(sl_);
  LOG(INFO) << "Time: " << kNumLockUnlock << " lock/unlock pairs "
            << "for RWMutex/SpinLock = " <<  durM << "/" << durS 
            << kUnitStr << ": SpeedUp = " 
            << static_cast<double>(durM)/static_cast<double>(durS);
  
  return;
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  SpinLockTester test{};
  test.LockBasicTest();
  test.LockExclusiveTest();
  test.LockSharedTest();
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
