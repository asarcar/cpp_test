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
#include <array>            // std::array
#include <random>           // std::distribution, random engine, ...
#include <thread>           // std::thread
#include <vector>           // std::vector
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/clock.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/concur.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);
DECLARE_int32(num_incs);

constexpr int NUM_THS           = 32;
// # of times we change value
constexpr int NUM_INCS          = 1024*1024;
// change of value a random number in {1, +MAX_RANGE} range
constexpr int MAX_RANGE         = 8;   

class Counter {
 public:
  Counter(): val_{} {}
  ~Counter() = default;

  inline void    Delta(int n) { val_ += n; }
  inline int     Get(void) { return val_; }
  inline void    Clear(void) { val_ = 0; }

 private:
  int            val_;
};

class ConcurTester {
 public:
  using Fut   = std::future<int>;
  using Th    = std::thread;
  using Aint  = std::atomic_int;

  ConcurTester() : ai_{},  
    rintFn_{bind(UniformIntDist{1,MAX_RANGE},
                 default_random_engine{random_device{}()})} {}
  ~ConcurTester() = default;

  void   SanityCheck(void);
  void   BasicTest(void);
  void   AdvancedTest(void);
  
 private:
  using RintGenFn       = function<int(void)>;
  using UniformIntDist  = uniform_int_distribution<int>;
  using ConcurCounter   = Concur<Counter>; 

  Aint          ai_;
  RintGenFn     rintFn_;

  static constexpr int kSleepDuration = 10000;
};

constexpr int ConcurTester::kSleepDuration;

void ConcurTester::SanityCheck(void) {
  Counter c{};
  c.Delta(-4);
  CHECK_EQ(c.Get(), -4);
  c.Delta(+5);
  CHECK_EQ(c.Get(), 1);
  c.Clear();
  CHECK_EQ(c.Get(), 0);

  LOG(INFO) << "Sanity Check: Passed for Counter";

  ConcurCounter cc{Counter{}};
  cc([](Counter& c2){
      c2.Delta(-4);
      CHECK_EQ(c2.Get(), -4);
      c2.Delta(+5);
      CHECK_EQ(c2.Get(), 1);
      c2.Clear();
      CHECK_EQ(c2.Get(), 0);
    });
  
  LOG(INFO) << "Sanity Check: Passed for Concurrent Counter";

  return;
}

// Check basic counter operations of Concur Class
// 1. Decrement and Increment works as expected
// 2. Clear works as expected
// 3. Any operation inside function passed to Concur
//    class is executed asynchronously. 
//    Destructor of Concur class execution completes
void ConcurTester::BasicTest(void) {
  Clock::TimePoint start;
  {
    ConcurCounter cc{Counter{}};
    // 1.
    Fut v1 = cc([](Counter& c){
        c.Delta(-4);
        c.Delta(+5);
        return c.Get();
      });
    CHECK(v1.valid());
    CHECK_EQ(v1.get(), 1);
    
    LOG(INFO) << "Basic Test: Passed Future Value pass/get with Inc/Dec";

    // 2.
    Fut v2 = cc([](Counter& c) {
        c.Clear();
        return c.Get();
      });
    CHECK_EQ(v2.get(), 0);

    LOG(INFO) << "Basic Test: Passed Future Value pass/get with Clear";

    // 3.
    start = Clock::USecs();
    cc([](Counter& c) {
        // sleep for some time
        std::this_thread::sleep_for(Clock::TimeUSecs(kSleepDuration)); 
      });

    CHECK_LT(Clock::USecs() - start, kSleepDuration);
  }

  LOG(INFO) << "Basic Test: Passed asynch Execution of Counter in ConcurCounter";

  CHECK_GT(Clock::USecs() - start, kSleepDuration);
           
  LOG(INFO) << "Basic Test: Passed Thread Completion on ConcurCounter Destruction";

  return;
}

void ConcurTester::AdvancedTest(void) {
  int cval;
  int ccval;
  array<Fut, NUM_THS> futs;

  {
    Counter       c{};
    ConcurCounter cc{Counter{}};

    array<Th, NUM_THS>  ths;

    int th_num = 0;
    for(auto &th : ths) {
      th = thread(
          [&c, &cc, &futs, this, th_num]() {
            int deltaVal = rintFn_();
            auto fn = [deltaVal](Counter &cnt){
              int acc=0;
              for(int i=0; i<FLAGS_num_incs; ++i) {
                cnt.Delta(deltaVal);
                acc += deltaVal;
              }
              return acc;
            };
            futs.at(th_num) = std::move(cc(fn));
            ai_ += fn(c);
          });
      ++th_num;
    }

    for(auto& th : ths) {
      th.join();
    }

    cval  = c.Get(); 
    // This would be the last function enQd to helper thread
    // As such the result would be the appropriate value
    Fut fval = cc([](Counter &c){ return c.Get(); });
    ccval = fval.get();
  }
  // destructor of Concur class called: would terminate helper thread loop 

  int expval = ai_.load();

  LOG(INFO) << "Vanilla Counter: Expecting " << expval 
            << ": Actual " << cval; 
  LOG(INFO) << "Concurrent Counter: Expecting " << expval 
            << ": Actual Counter Value " << ccval; 
  CHECK_EQ(ccval, expval);

  int ccval2 = 0;
  for (int i=0; i<NUM_THS; ++i)
    ccval2 += futs.at(i).get();

  LOG(INFO) << "Concurrent Counter: Expecting " << expval 
            << ": Accumulated Future Value " << ccval2; 
  CHECK_EQ(ccval2, expval);

  LOG(INFO) << "Advanced Test: Passed Synch Ops on ConcurrentCounter";

  return;
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  ConcurTester test{};
  test.SanityCheck();
  test.BasicTest();
  test.AdvancedTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_int32(num_incs, NUM_INCS, 
             "number of times we change value"); 
