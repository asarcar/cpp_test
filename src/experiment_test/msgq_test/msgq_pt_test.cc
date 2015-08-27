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
#include <algorithm>   // std::min
#include <future>      // std::future, std::packaged_task
#include <iostream>
#include <numeric>     // std::accumulate
#include <sstream>     // ostringstream
#include <thread>      // std::thread
#include <vector>
// Standard C Headers
#include <cstdlib>     // std::exit, EXIT_FAILURE
#include <cassert>     // assert
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/clock.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/concur_block_q.h"

using namespace std;
using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;

class ServerPool {
 public:
  using Fn = uint32_t(std::vector<uint32_t>*, uint32_t, uint32_t);
  using PackageTaskType = std::packaged_task<Fn>;
  struct QElem {
    std::vector<uint32_t>*          val_p;
    uint32_t                        first;
    uint32_t                        last;
    PackageTaskType                 pt;

    QElem():val_p{}, first{}, last{}, pt{} {};
    QElem(std::vector<uint32_t>* v_p, uint32_t f, uint32_t l, PackageTaskType &&p) :
        val_p{v_p}, first{f}, last{l}, pt{std::move(p)} {};
    ~QElem() = default;

    QElem(QElem &&o) : 
        val_p{o.val_p}, first{o.first}, last{o.last}, pt{std::move(o.pt)} {}
    QElem& operator=(QElem&& o) {
      val_p = o.val_p; first = o.first; last = o.last; pt = std::move(o.pt);
      return *this;
    }

    QElem(const QElem& o) = delete;
    QElem& operator=(const QElem& o) = delete;
  };

  explicit ServerPool(uint32_t num_threads) {
    LOG(INFO) << "Main TH " << std::hex << std::this_thread::get_id() 
              << ": ServerPool called with num_threads " << num_threads;

    for (int i=0; i<static_cast<int>(num_threads); i++) {
      std::thread t = 
          std::thread(    
              [this]()->void {
                while (true) {
                  QElem e = this->_sync_msg_q.Pop();
                  LOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
                            << ": QElem popped: val " 
                            << std::hex << e.val_p->data()
                            << std::dec << ": range: [" 
                            << e.first << "," << e.last << ")";
                  // dummy range: signal terminate thread
                  if (e.first == e.last) {
                    LOG(INFO) << "TH " << std::hex << std::this_thread::get_id() << " terminated!";
                    return;
                  }
                  e.pt(e.val_p, e.first, e.last);
                }
                return;
              });
      LOG(INFO) << "TH " << std::hex << t.get_id() << " created as ServerPool to process tasks";
      _th_pool.push_back(std::move(t));
    }
    return;
  }
      
  ~ServerPool(void)                         = default;
  ServerPool(const ServerPool&)             = delete;
  ServerPool& operator=(const ServerPool &) = delete;
  ServerPool(ServerPool&&)                  = delete;
  ServerPool& operator=(ServerPool &&)      = delete;

  // Interface used by users to submit task to ServerPool
  void submit_task(QElem &&e) {
    _sync_msg_q.Push(std::move(e));
    return;
  }
  // Ensure that all threads have completed termination from system perspective 
  void join_threads(void);
 private:
  ConcurBlockQ<QElem>                       _sync_msg_q;
  std::vector<std::thread>                  _th_pool;
};

class ServerPoolTest {
 public:
  using FutureType = std::future<uint32_t>;

  explicit ServerPoolTest(int argc, char **argv); 
  ~ServerPoolTest(void)                               = default;

  ServerPoolTest(const ServerPoolTest &at)            = delete;
  ServerPoolTest(ServerPoolTest &&at)                 = delete;
  ServerPoolTest& operator=(const ServerPoolTest &at) = delete;
  ServerPoolTest& operator=(ServerPoolTest &&at)      = delete;

  void submit_tasks(void);
  uint32_t compute_sum(void);
  // Ensure that all threads have completed termination from system perspective 
  void join_threads(void);
  void disp_state(void);
 private:
  constexpr static uint32_t DEF_SIZ = 128;
  constexpr static uint32_t MIN_SIZ = 32; // don't parallelize unless size > 32

  uint32_t                  _num_ths;
  ServerPool                _sp;

  uint32_t                  _siz    = DEF_SIZ;
  uint32_t                  _range  = 0;
  std::vector<uint32_t>     _vals   = {};
  std::vector<uint32_t>     _lims   = {};
  std::vector<FutureType>   _fu_pool= {};
  uint32_t                  _sum     = 0;

  constexpr static uint32_t get_num_ranges(uint32_t siz) {
    return (siz + MIN_SIZ - 1)/MIN_SIZ;
  }

  // Initialize the ranges each task will process
  inline void init_limits(void) {
    uint32_t last_range=0;
    for (auto &lim:_lims) {
      lim = last_range; 
      last_range += MIN_SIZ;
    }
    
    // last range cannot exceed siz
    if (_lims.at(_range) > _siz)
      _lims.at(_range) = _siz;

    return;
  }
};

ServerPoolTest::ServerPoolTest(int argc, char **argv) : 
    _num_ths{std::thread::hardware_concurrency()}, _sp{_num_ths}
{
  // Parse Arguments...
  if (argc == 1) {
    _siz = DEF_SIZ;
  } else {
    int32_t s = 0;
    if (argc >= 2)
      s = std::stoi(std::string(argv[1]));
    if ((s < 0) || (argc > 2)) {
      std::ostringstream oss;
      oss << "Usage: " << argv[0] << " [arr_siz]" << std::endl;
      throw oss.str();
    }
    _siz = s;
  }

  // Break the array into elements of 32 elements each
  _range = get_num_ranges(_siz);


  // Initialize Vectors...
  // Value Vector
  _vals.resize(_siz);
  int i=0;
  for (auto &val: _vals)
    val = ++i;
  // Limit Vector: reserve one additional entry needed for the last range
  _lims.resize(_range + 1);
  init_limits();

  LOG(INFO) << "Vector " << std::hex << _vals.data() << ": size " << _vals.size() 
            << ": broken num_ranges " << _range;

  return;
}

void ServerPoolTest::submit_tasks(void) {
  // Create tasks for each range i.e. a computation unit
  std::function<ServerPool::Fn> acc_fn = 
      [](std::vector<uint32_t>* v_p, 
         uint32_t first, uint32_t last) -> uint32_t {
    uint32_t val = std::accumulate(v_p->begin() + first, v_p->begin() + last, 0);
    LOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
    << " SUM of vector " << std::hex << v_p->data() 
    << ": compute range [" << std::dec
    << first  << "," << last << ") = " << val;
    return val;
  };

  // Remember future from each PackageTask and store in vector
  // so that the value can be extracted for computing sum
  for (int i=0; i<static_cast<int>(_range); ++i) {
    ServerPool::PackageTaskType pt{acc_fn};
    _fu_pool.push_back(pt.get_future());
    ServerPool::QElem e = {&_vals, _lims[i], _lims[i+1], std::move(pt)};
    _sp.submit_task(std::move(e));
  }

  // Now that all computation requests are inserted
  // create dummy computation terminate requests
  for (uint32_t i=0; i<_num_ths; ++i) {
    // Packaged Task: default ctor does not hold any task: no exception
    ServerPool::QElem e = {&_vals, 0, 0, ServerPool::PackageTaskType{}};
    _sp.submit_task(std::move(e));
  }

  return;
}

uint32_t ServerPoolTest::compute_sum(void) {
  _sum = 0;
  for (auto &fu : _fu_pool) {
    _sum += fu.get();
  }

  LOG(INFO) << "MainThread TH " << std::hex << std::this_thread::get_id() 
            << ": num_ranges " << std::dec
            << _fu_pool.size() << ": sub-sum " << _sum;

  return _sum;
}

void ServerPool::join_threads(void) {
  for (auto &t:_th_pool) {
    LOG(INFO) << "TH " << std::hex << t.get_id() << " joining main...";
    t.join();
  }
  return;
}

// Ensure that all threads have completed termination from system perspective 
void ServerPoolTest::join_threads(void) {
  // Thread Pool is managed by the ServerPool Class. 
  // Request Join for all spawned threads
  _sp.join_threads();
  return;
}

void ServerPoolTest::disp_state(void) {
  DLOG(INFO) << "Size " << _siz << ": Range " << _range 
             << ": NumThs " << _num_ths << std::endl;
  
  DLOG(INFO) << "Values: 1.." << _vals.size() << ": in vector of size " << _vals.size();
  DLOG(INFO) << "Broken in #Ranges " << _lims.size();
  int i=0;
  for (auto &val: _lims) {
    DLOG(INFO) << ": [" << i++ << "]" << val;
  }
  DLOG(INFO) << "Total Sum " << _sum;
  
  return;
}

int main(int argc, char **argv) {
  asarcar::Init::InitEnv(&argc, &argv);
  try {
    ServerPoolTest spt(argc, argv);
    
    Clock::TimePoint start = Clock::USecs();
    spt.submit_tasks();
    uint32_t sum = spt.compute_sum();
    Clock::TimeDuration dur = Clock::USecs() - start;
    spt.join_threads();
    spt.disp_state();
    std::cout << "Computed Sum " << sum << " in " << dur << " usecs" << std::endl;
  }
  catch(const std::string &s) {
    LOG(WARNING) << "String Exception caught: " << s << std::endl;
    return -1;
  }
  catch(std::exception &e) {
    LOG(WARNING) << "Exception caught: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
