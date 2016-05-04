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
#include <functional>       // std::bind, std::function
#include <iostream>
#include <random>           // std::distribution, random engine, ...
#include <thread>           
#include <vector>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/clock.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/lock_guard.h"
#include "utils/concur/rw_lock.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;

// Declarations
DECLARE_bool(auto_test);
DECLARE_int32(num_threads);
DECLARE_int32(num_reads_per_write);

constexpr int NUM_THS = 32;
constexpr int NUM_RDS_PER_WR = 7; // 1 in 8 Ops are Wr: rest are Rd
constexpr int MAX_NUM_WAIT = 100;

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_int32(num_threads, NUM_THS, 
             "number of threads spawned for testing"); 
DEFINE_int32(num_reads_per_write, NUM_RDS_PER_WR, 
             "number of reads for every write"); 
class ValTest {
public:
  using RWSpinLock = RWLock<SpinLock>;
  void op(LockMode mode, int inc) {
    uint32_t rnd_val = _wfn();
    LockGuard<RWSpinLock> lckg{_rwm, mode};
    if (mode == LockMode::EXCLUSIVE_LOCK) {
      _v += inc;
    } 
    std::this_thread::sleep_for(Clock::TimeMSecs(rnd_val));
    LOG(INFO) << "TH " << std::hex << "0x" << std::this_thread::get_id() 
              << ": OP=" << mode
              << ": val=" << std::dec << _v 
              << ": after " << rnd_val << " millisecs";
    return;
  }
  void disp(void) {
    LOG(INFO) << "TH: " << std::hex << "0x" << std::this_thread::get_id() 
              << " val=" << std::dec <<  _v << std::endl;
    CHECK_EQ(_v, 76) << "val=" << _v << " != 76 (expected_val): Error!" 
                     << std::endl;
    return;
  }
 private:
  int      _v{0};
  RWSpinLock _rwm{};
  using RdValFn=uint32_t(void);
  std::function<uint32_t(void)> 
  _wfn { std::bind(std::uniform_int_distribution<uint32_t>{1,MAX_NUM_WAIT}, 
                   std::default_random_engine{std::random_device{}()}) };
};

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  ValTest val_test{};

  std::vector<std::thread> th_pool;

  // every FLAGS_num_reads_per_write + 1 will have 1 write: rest are reads
  int num_rd_wr_quanta = FLAGS_num_reads_per_write + 1;
  int cnt = 0;

  for (int i=0; i<FLAGS_num_threads; ++i) {
    LockMode mode{LockMode::SHARE_LOCK};
    if (((i + 1) % num_rd_wr_quanta) == 0)
      mode = LockMode::EXCLUSIVE_LOCK;

    th_pool.push_back(std::thread([&val_test](LockMode mode, int inc) {
          val_test.op(mode, inc);
        }, mode, i));
  }   
  // Join all spawned threads
  for(auto &t:th_pool)
    t.join();
  
  val_test.disp();
  
  return 0;
}
