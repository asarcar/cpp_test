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
#include <chrono>      // std::chrono
#include <deque>       // std::deque
#include <future>      // std::promise, std::future
#include <iostream>
#include <numeric>     // std::accumulate
#include <sstream>     // ostringstream
#include <vector>
// Standard C Headers
#include <cstdlib>     // std::exit, EXIT_FAILURE
#include <cassert>     // assert
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

template <typename T>
class MsgQ {
 public:
  MsgQ()                         = default;
  ~MsgQ()                        = default;
  MsgQ(const MsgQ& o)            = delete;
  MsgQ& operator=(const MsgQ& o) = delete;
  MsgQ(MsgQ&& o): 
      _cv{std::move(o._cv)}, _m{std::move(o._m)}, _q{std::move(o._q)} {}
  MsgQ& operator=(MsgQ&& o) {
    _cv = std::move(o._cv);
    _m = std::move(o._m);
    _q = std::move(o._q);
  }
  void pushQ(T&& val) {
    {
      std::lock_guard<std::mutex> lck{_m};
      _q.push_back(std::move(val));
    }
    // unlocked before notify so that we avoid unnecessary locking of the woken task
    _cv.notify_one();
  }
  T popQ(void) {
    std::unique_lock<std::mutex> lck{_m};
    _cv.wait(lck, [this](){return !this->_q.empty();});
    T res = std::move(_q.front());
    _q.pop_front();
    return res;
  }
 private:
  std::condition_variable _cv;
  std::mutex              _m;
  std::deque<T>           _q;  
};    
    

class MsgQTest {
 public:
  explicit MsgQTest(int argc, char **argv); 
  ~MsgQTest() = default;

  MsgQTest(const MsgQTest &at) = delete;
  MsgQTest(MsgQTest &&at) = delete;
  MsgQTest& operator=(const MsgQTest &at) = delete;
  MsgQTest& operator=(MsgQTest &&at) = delete;

  void submit_tasks(void);
  void spawn_threads(void);
  uint32_t compute_sum(void);
  // Ensure that all threads have completed termination from system perspective 
  void join_threads(void);
  void disp_state(void);
 private:
  const static uint32_t DEF_SIZ = 128;
  const static uint32_t MIN_SIZ = 32; // don't parallelize unless size > 32
  const static uint32_t NUM_THS = 8;

  uint32_t _sum = 0;
  uint32_t _siz = DEF_SIZ;
  uint32_t _range = 0;
  uint32_t _num_ths = 0;

  std::vector<uint32_t>              _vals;
  std::vector<uint32_t>              _lims;
  std::vector<std::thread>           _th;

  using req_range_t=std::pair<uint32_t, uint32_t>;
  MsgQ<req_range_t>                  _msgQ;
  using res_range_t=std::pair<std::thread::id, uint32_t>;
  MsgQ<res_range_t>                  _resMsgQ;

  inline uint32_t get_num_ranges(void) const {
    return (_siz + MIN_SIZ - 1)/MIN_SIZ;
  }

  // Initialize the ranges each thread will process
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

// allocate memory for an outside referenced static variable
const uint32_t MsgQTest::NUM_THS;


MsgQTest::MsgQTest(int argc, char **argv) {
  // Parse Arguments...
  if (argc == 1) {
    _siz = DEF_SIZ;
  } else {
    int32_t s;
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
  _range = get_num_ranges();
  _num_ths = std::min(_range, NUM_THS);

  // Initialize Vectors...
  // Value Vector
  _vals.resize(_siz);
  uint32_t i=0;
  for (auto &val: _vals)
    val = ++i;
  // Limit Vector: reserve one additional entry needed for the last range
  _lims.resize(_range + 1);
  init_limits();

  return;
}

void MsgQTest::submit_tasks(void) {
  // Create msgQ computation request entries for threads
  for (uint32_t i=0; i<_range; ++i) {
    req_range_t req = std::make_pair(_lims[i], _lims[i+1]);
    // std::cout << "Generating task range idx " << i 
    //           << " [" << _lims[i] << "," << _lims[i+1] << ")" << std::endl;
    _msgQ.pushQ(std::move(req));
  }

  // Now that all computation requests are inserted
  // create dummy computation terminate requests
  for (uint32_t i=0; i<_num_ths; ++i) {
    req_range_t req_term_th = std::make_pair(0,0);
    _msgQ.pushQ(std::move(req_term_th));
  }

  return;
}

void MsgQTest::spawn_threads(void) {
  for (uint32_t i=0; i<_num_ths; ++i) {
    // Spawn the Thread
    std::thread t([this](const std::vector<uint32_t>& vals) {
        // Deque all computation request msgQ entries 
        // Compute the sum and enque results to resMsgQ
        for (;;) {
          req_range_t req = this->_msgQ.popQ();
          // compute the sum of this range and enter the result 
          uint32_t val = std::accumulate(vals.begin() + req.first,
                                         vals.begin() + req.second, 0);
          // std::cout << "ThID " << std::this_thread::get_id()
          //           << ": range [" << req.first << "," << req.second << ")"
          //           << ": partial sum " << val << std::endl;
          // submit the result in resMsgQ
          res_range_t res = std::make_pair(std::this_thread::get_id(), val);
          this->_resMsgQ.pushQ(std::move(res));

          // dummy entry with range.first == range.second == 0 => terminate thread
          if ((req.first == 0) && (req.second == 0))
            break;
        }
        return;
      }, this->_vals);
    _th.push_back(std::move(t));
  }
  
  return;
}

uint32_t MsgQTest::compute_sum(void) {
  _sum = 0;

  for (uint32_t num_terminated_threads = 0; num_terminated_threads < _num_ths;) {
    res_range_t res = this->_resMsgQ.popQ();
    // std::cout << "ThID " << res.first << " msgd partial sum " << res.second << std::endl;
    _sum += res.second;
    // Thread i computation over
    if (res.second == 0) {
      ++num_terminated_threads;
      // std::cout << "ThID " <<  res.first << ": termination ack received" 
      //           << ": # threads terminated " << num_terminated_threads 
      //           << ": total threads " << _num_ths << std::endl;
    }
  }

  return _sum;
}

// Ensure that all threads have completed termination from system perspective 
void MsgQTest::join_threads(void) {
  for (auto &t:_th)
    t.join();

  return;
}

void MsgQTest::disp_state(void) {
  std::cout << "Size " << _siz << ": Range " << _range 
            << ": NumThs " << _num_ths << std::endl;

  std::cout << "Values: #elem " << _vals.size() << std::endl;
  uint32_t i=0;
  for (auto &val: _vals) {
    std::cout << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Range: #elem " << _lims.size() << std::endl;
  i=0;
  for (auto &val: _lims) {
    std::cout << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Total Sum " << _sum << std::endl;

  return;
}

int main(int argc, char **argv) {
  try {
    MsgQTest mt(argc, argv);
    
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    mt.submit_tasks();
    mt.spawn_threads();
    uint32_t sum = mt.compute_sum();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    std::chrono::milliseconds dur = 
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    mt.join_threads();
    // mt.disp_state();
    std::cout << "Computed Sum " << sum << " in " << dur.count() << " milli secs" << std::endl;
  }
  catch(const std::string &s) {
    std::cerr << "String Exception caught: " << s << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return 0;
}
