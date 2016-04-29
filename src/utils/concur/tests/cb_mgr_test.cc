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
#include "utils/concur/cb_mgr.h"
#include "utils/concur/cv_guard.h"
#include "utils/concur/spin_lock.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

struct SharedState {
  atomic_int num{0};
};

class TestHelper {
 public:
  TestHelper(SharedState &s) : s_(s), sl_{}, cv_{sl_}, go_{false} {}
  ~TestHelper() {
    CB_QUASH_N_WAIT(cb_mgr_);
  }
  static void Incrementor(TestHelper *p) {
    CV<SpinLock>::WaitGuard{p->cv_, [p](){return p->go_;}};
    ++p->s_.num;
  }
  void BlockOps(void) {
    LockGuard<SpinLock> lkg{sl_};
    go_ = false;
  }
  void EnableOps(void) {
    CV<SpinLock>::SignalGuard{cv_};
    go_ = true;
  }
  CbMgr::Closure AddClosure(CbMgr::Closure&& fn) {
    return cb_mgr_.Seal(forward<CbMgr::Closure>(fn));
  }
 private:
  SharedState&      s_;
  SpinLock          sl_;
  CV<SpinLock>      cv_;
  bool              go_;
  CbMgr             cb_mgr_;
};

class CbMgrTester {
 public:
  void SanityTest(void);
  void ConcurTest(void);
 private:
  static constexpr uint32_t kSleepDuration = 10000;
};

constexpr uint32_t CbMgrTester::kSleepDuration;

void CbMgrTester::SanityTest(void) {
  SharedState s;
  CbMgr::Closure cb1, cb2;
  // Closures are called as expected 
  {
    TestHelper  t{s};
    t.EnableOps();
    cb1 = t.AddClosure(bind(&TestHelper::Incrementor, &t));
    cb2 = t.AddClosure(bind([&s](){s.num+=2;}));
    cb1();
    CHECK_EQ(s.num, 1);
    cb2();
    CHECK_EQ(s.num, 3);  
  }
  // TestHelper Destroyed: CbMgr Destroyed
  // New Closure calls cb1() & cb2() are allowed but as noop
  {
    cb1();
    CHECK_EQ(s.num, 3);  
    cb2();
    CHECK_EQ(s.num, 3);  
  }
}

// Pending Closure called before QUASH called allowed to execute
// and QUASH does not complete until Pending Closure completes.
// New Closure calls after QUASH not allowed to execute
void CbMgrTester::ConcurTest(void) {
  SharedState s;
  TestHelper  *p = new TestHelper{s};
  unique_ptr<TestHelper> t_p{p};

  CbMgr::Closure cb1 = p->AddClosure(bind(&TestHelper::Incrementor, p));
  CbMgr::Closure cb2 = p->AddClosure(bind([&s](){s.num+=2;}));

  // CB (Incrementor) cannot proceed as it is awaiting signal
  thread th1 = thread(cb1), th2 = thread(cb2);
  thread th3 = thread([p](){
      // sleep for some time to allow destructor to be called first
      this_thread::sleep_for(Clock::TimeUSecs(2*kSleepDuration)); 
      p->EnableOps();
    });

  Clock::TimePoint start = Clock::USecs();

  // sleep for some time to allow cb1 and cb2 thread to execute
  this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
  CHECK_EQ(s.num, 2);
  // TestHelper Destroy delayed until th3 EnableOps => th1 cb1 completes
  t_p.reset();
  CHECK_EQ(s.num, 3); // check cb1 was completed

  Clock::TimePoint destroy_time = Clock::USecs() - start;

  th1.join();
  th2.join();
  th3.join();

  CHECK_GT(destroy_time, 2*kSleepDuration); 
  
  // any further calls of callback allowed but ignored.
  cb1(); cb2();
  CHECK_EQ(s.num, 3);
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);
  
  CbMgrTester cbt;
  cbt.SanityTest();
  cbt.ConcurTest();

  LOG(INFO) << "Callback Manager Tests Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
