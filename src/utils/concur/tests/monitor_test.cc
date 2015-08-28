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
#include "utils/concur/monitor.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);
DECLARE_int32(num_incs);

constexpr int NUM_THS           = 32;
// # of times we change value
constexpr int NUM_INCS          = 1024;
// loop modify on a given changed value in range {1, NUM_LOOP_DELTA} range
constexpr int NUM_LOOP_DELTA    = 32;
// change of value a random number in {-MAX_RANGE, +MAX_RANGE} range
constexpr int MAX_RANGE         = 16;   

class Counter {
 public:
  Counter(): _val{} {}
  ~Counter() = default;

  inline void    Delta(int n) { _val += n; }
  inline int32_t Get(void) { return _val; }
  inline void    Clear(void) { _val = 0; }

 private:
  int32_t           _val;
};

class CounterTest {
 public:
  CounterTest();
  ~CounterTest() = default;

  void   SanityCheck(void);
  void   BasicTest(void);
  void   AdvancedTest(void);

  atomic<int32_t>   atomic_val;
 private:
  using RintGenFn       = function<int32_t(void)>;
  using UniformIntDist  = uniform_int_distribution<int32_t>;
  using RuintGenFn      = function<uint32_t(void)>;
  using UniformUintDist = uniform_int_distribution<uint32_t>;

  using MonCounter      = Monitor<Counter>; 

  RintGenFn     _rintFn;
  RuintGenFn    _ruintFn;

  Counter       _c;
  MonCounter    _mc;
};

CounterTest::CounterTest() :
    atomic_val{},
  _rintFn{bind(UniformIntDist{-MAX_RANGE,MAX_RANGE},
               default_random_engine{random_device{}()})},
  _ruintFn {bind(UniformUintDist{1,NUM_LOOP_DELTA},
                 default_random_engine{random_device{}()})},
  _c{}, _mc{Counter{}} {
}

void CounterTest::SanityCheck(void) {
  _c.Delta(-4);
  CHECK_EQ(_c.Get(), -4);

  _c.Delta(+5);
  CHECK_EQ(_c.Get(), 1);

  _c.Clear();
  CHECK_EQ(_c.Get(), 0);

  return;
}

void CounterTest::BasicTest(void) {
  auto v1 = 
      _mc([](Counter& c){
          c.Delta(-4);
          c.Delta(+5);
          return c.Get();
        });
  CHECK_EQ(v1, 1);

  auto v2 = 
      _mc([](Counter& c) {
          c.Clear();
          return c.Get();
        });

  CHECK_EQ(v2, 0);

  return;
}

void CounterTest::AdvancedTest(void) {
  vector<thread> ths{NUM_THS};

  for(auto &th : ths) {
    th = 
        thread(
            [=]() {
              for(int i = 0; i < FLAGS_num_incs; ++i) {
                int delVal = _rintFn();
                int numDel = _ruintFn();
            
                for(int j = 0; j < numDel; ++j) {
                  _c.Delta(delVal);
                }

                for(int j = 0; j < numDel; ++j) {
                  _mc([delVal](Counter& c){
                      c.Delta(delVal);
                    });
                }

                atomic_val.fetch_add(numDel*delVal);
              }
            });
  }

  for(auto& th : ths) {
    th.join();
  }

  LOG(INFO) << "Vanilla Counter: Expecting " << atomic_val << ": Actual " << _c.Get(); 
  auto v = _mc([](Counter &c){ return c.Get(); });
  LOG(INFO) << "Monitored Counter: Expecting " << atomic_val << ": Actual " << v; 
  CHECK_EQ(v, atomic_val);

  return;
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  CounterTest test{};
  test.SanityCheck();
  test.BasicTest();
  test.AdvancedTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_int32(num_incs, NUM_INCS, 
             "number of times we change value"); 
