// Copyright 2016 asarcar Inc.
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
#include <algorithm>    // std::max
#include <mutex>
#include <thread>
// Standard C Headers
#include <cxxabi.h>
// Google Headers
#include <glog/logging.h>
// Local Headers
#include "utils/basic/clock.h"
#include "utils/basic/init.h"
#include "utils/concur/cv_guard.h"
#include "utils/concur/rw_lock.h"
#include "utils/concur/spin_lock.h"

 using namespace asarcar;
 using namespace asarcar::utils;
 using namespace asarcar::utils::concur;
 using namespace std;

 // Declarations
 DECLARE_bool(auto_test);

 static constexpr int kSleepMSecs = 60, 
   kDelayMSecs                    = 40, 
   k2DelayMSecs                   = 2*kDelayMSecs,
   kSleepNDelayMSecs              = kSleepMSecs + kDelayMSecs,
   kSleepN2DelayMSecs             = kSleepMSecs + 2*kDelayMSecs,
   kSleepN3DelayMSecs             = kSleepMSecs + 3*kDelayMSecs;

static constexpr int kNumThreads  = 3;

template<typename LockType, LockMode Mode=LockMode::EXCLUSIVE_LOCK>
struct CvGuardTester {
  LockType              lck;
  CV<LockType,Mode>     cv;

  CvGuardTester() : lck{}, cv{lck} {}
  ~CvGuardTester() = default;
  // Case 1: Wait precedes signal
  void WaitTest(void) {
    Clock::TimePoint now = Clock::MSecs();
    std::atomic_bool cond{false};
    std::atomic_int  count{0};
    std::thread tid = std::thread([this, &cond, &count]() {
        // introduce Delay to allow waiter to proceed first
         std::this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
         ++count;
         {
           CvSg<LockType,Mode> sg{cv};
           cond = true;
           std::this_thread::sleep_for(Clock::TimeMSecs(kSleepMSecs));
           ++count;
         }
         std::this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
         ++count;
      });
    CHECK_EQ(count, 0);
    CvWg<LockType,Mode> wg{cv, [&cond](){return cond.load();}};
    CHECK_EQ(count, 2);
    Clock::TimeDuration elapsed1 = Clock::MSecs() - now;
    CHECK_GE(elapsed1, kSleepNDelayMSecs);
    CHECK_LE(elapsed1, kSleepN2DelayMSecs);
    tid.join();
    CHECK_EQ(count, 3);
    Clock::TimeDuration elapsed2 = Clock::MSecs() - now;
    CHECK_GE(elapsed2, kSleepN2DelayMSecs);
    CHECK_LE(elapsed2, kSleepN3DelayMSecs);
  }

  // Case 2: Signal precedes Wait
  void NotifyTest(void) {
    Clock::TimePoint now = Clock::MSecs();
    std::atomic_bool cond{false};
    std::atomic_int  count{0};
    std::thread tid = std::thread([this, &cond, &count]() {
        ++count;
        {
          CvSg<LockType,Mode> sg{cv};
          cond = true;
          ++count;
        }
        std::this_thread::sleep_for(Clock::TimeMSecs(kSleepMSecs));
        ++count;
      });
    // introduce Delay to allow Signal to proceed first
    std::this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
    CHECK_EQ(count, 2);
    CvWg<LockType,Mode> wg{cv, [&cond](){return cond.load();}};
    CHECK_EQ(count, 2);
    Clock::TimeDuration elapsed1 = Clock::MSecs() - now;
    CHECK_GE(elapsed1, kDelayMSecs);
    CHECK_LE(elapsed1, k2DelayMSecs);
    tid.join();
    CHECK_EQ(count, 3);
    Clock::TimeDuration elapsed2 = Clock::MSecs() - now;
    CHECK_GE(elapsed2, std::max(kSleepMSecs, kDelayMSecs));
    CHECK_LE(elapsed2, kSleepNDelayMSecs);
  }
  
  // Case 3: Many Waiters wait simultaneously. 
  //         Signallers {uni/broad}cast to Waiters
  void NotifyAllTest(void) {
    std::atomic_bool cond{false};
    std::atomic_int  prewaiters{0}, waiters{0};
    auto signal_fn = [this, &cond](bool broadcast) {
      CvSg<LockType,Mode> sg{cv, broadcast};
      cond = true;
    }; 
    auto wait_fn = [this, &cond, &prewaiters, &waiters](int i) {
      ++prewaiters;
      CvWg<LockType,Mode> wg{cv, [&cond](){return cond.load();}};
       ++waiters;
    };
    
    // When signal is not broadcast then only one waiter is allowed to proceed
    array<thread, kNumThreads> th_id;
    for (int i=0; i<kNumThreads; ++i)
      th_id.at(i) = thread(wait_fn, i);
    // allow waiters time to proceed first
    this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
    CHECK_EQ(prewaiters, kNumThreads); // validate all threads are waiting
    CHECK_EQ(waiters, 0); // validate all threads are waiting
    signal_fn(false);
    // unicast signal: allow time to wake one waiter
    this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
    CHECK_EQ(waiters, 1); // check only one waiter proceeded
    // bcast signal: allow time to wake remaining waiters
    signal_fn(true);
    this_thread::sleep_for(Clock::TimeMSecs(kDelayMSecs)); 
    CHECK_EQ(waiters, kNumThreads); // validate all threads proceeded
    for (int i=0; i<kNumThreads; ++i) 
      th_id.at(i).join();
    
    // Now that the cond is true: check waiters proceed without any signal
    for (int i=0; i<kNumThreads; ++i)
       th_id.at(i) = thread(wait_fn, i);
    for (int i=0; i<kNumThreads; ++i) 
      th_id.at(i).join();
    CHECK_EQ(prewaiters, 2*kNumThreads);
    CHECK_EQ(waiters, 2*kNumThreads);
  }  
};

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  CvGuardTester<SpinLock, LockMode::SHARE_LOCK> test_sh_sp{};
  test_sh_sp.WaitTest();
  test_sh_sp.NotifyTest();
  test_sh_sp.NotifyAllTest();
  LOG(INFO) << "Condition Variable<SpinLock,Share_Lock> Passed";

  if (FLAGS_auto_test)
    return 0;

  CvGuardTester<SpinLock> test_sp{};
  test_sp.WaitTest();
  test_sp.NotifyTest();
  LOG(INFO) << "Condition Variable<SpinLock,Exclusive_Lock> Passed";

  CvGuardTester<mutex> test_m{};
  test_m.WaitTest();
  test_m.NotifyTest();
  LOG(INFO) << "Condition Variable<Mutex,Exclusive_Lock> Passed";

  CvGuardTester<RWLock<mutex>, LockMode::SHARE_LOCK> test_sh_rw_mu{};
  test_sh_rw_mu.WaitTest();
  test_sh_rw_mu.NotifyTest();
  test_sh_rw_mu.NotifyAllTest();
  LOG(INFO) << "Condition Variable<RWLock<mutex>,Share_Lock> Passed";

  CvGuardTester<RWLock<mutex>, LockMode::EXCLUSIVE_LOCK> test_rw_mu{};
  test_rw_mu.WaitTest();
  test_rw_mu.NotifyTest();
  LOG(INFO) << "Condition Variable<RWLock<mutex>,Exclusive_Lock> Passed";

  CvGuardTester<RWLock<SpinLock>, LockMode::SHARE_LOCK> test_sh_rw_sp{};
  test_sh_rw_sp.WaitTest();
  test_sh_rw_sp.NotifyTest();
  test_sh_rw_sp.NotifyAllTest();
  LOG(INFO) << "Condition Variable<RWLock<SpinLock>,Share_Lock> Passed";

  CvGuardTester<RWLock<SpinLock>, LockMode::EXCLUSIVE_LOCK> test_rw_sp{};
  test_rw_sp.WaitTest();
  test_rw_sp.NotifyTest();
  LOG(INFO) << "Condition Variable<RWLock<SpinLock>,Exclusive_Lock> Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
