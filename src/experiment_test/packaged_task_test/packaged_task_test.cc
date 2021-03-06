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
#include <chrono>      // std::chrono
#include <exception>   // std::throw
#include <future>      // std::async
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

class PackagedTaskTest {
 public:
  explicit PackagedTaskTest(int argc, char **argv); 
  ~PackagedTaskTest() = default;

  PackagedTaskTest(const PackagedTaskTest &at) = delete;
  PackagedTaskTest(PackagedTaskTest &&at) = delete;
  PackagedTaskTest& operator=(const PackagedTaskTest &at) = delete;
  PackagedTaskTest& operator=(PackagedTaskTest &&at) = delete;

  void spawn_threads(void);
  uint32_t compute_sum(void);
  void join_threads(void);
  void disp_state(void);
 private:
  const static uint32_t DEF_SIZ = 128;
  const static uint32_t MIN_SIZ = 32; // don't parallelize unless size > 32

  uint32_t _sum = 0;
  uint32_t _siz = DEF_SIZ;
  uint32_t _range = 0;

  std::vector<uint32_t> _vals;
  std::vector<uint32_t> _lims;
  std::vector<std::future<uint32_t>> _future_sums;
  std::vector<std::thread> _th;

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

PackagedTaskTest::PackagedTaskTest(int argc, char **argv) {
  // Parse Arguments...
  if (argc == 1) {
    _siz = DEF_SIZ;
  } else {
    int32_t s;
    s = std::stoi(std::string(argv[1]));
    if ((s < 0) || (argc > 2)) {
      std::ostringstream oss;
      oss << "Usage: " << argv[0] << " [arr_siz]" << std::endl;
      throw oss.str();
    }
    _siz = s;
  }

  // Initialize Vector...
  _vals.resize(_siz);
  uint32_t i=0;
  for (auto &val: _vals)
    val = i++;

  // Break the array into elements of 32 elements each
  _range = get_num_ranges();
  // one additional entry needed for the last range
  _lims.resize(_range + 1);
  init_limits();

  return;
}

void PackagedTaskTest::spawn_threads(void) {
  // should not be called in succession
  // only call when future_sums and threads are not already spawned
  assert(_future_sums.size() == 0);
  assert(_th.size() == 0);
  
  // Set up the future array and spawn the async threads
  for (uint32_t j=0; j<_range; ++j) {
    std::packaged_task<uint32_t(void)> 
        pt([this, j]() {
            uint32_t val = std::accumulate(this->_vals.begin()+
                                           this->_lims.at(j), 
                                           this->_vals.begin()+
                                           this->_lims.at(j+1), 
                                           0);
            // std::cout << "ThID " << std::this_thread::get_id() 
            //           << ": idx=" << j << " [" 
            //           << _lims.at(j) << "," << _lims.at(j+1) << ")=" 
            //           << val << std::endl;
            return val;
          });
    _future_sums.push_back(pt.get_future());
    _th.push_back(std::thread(std::move(pt)));
  }

  return;
}

uint32_t PackagedTaskTest::compute_sum(void) {
  _sum=0;
  for (auto &fut:_future_sums)
    _sum += fut.get();
  
  // std::cout << "Array: #elems " << _siz << ": MainThID " 
  //           << std::this_thread::get_id() << ": Total_Sum " 
  //           << _sum << std::endl;

  return _sum;
}

void PackagedTaskTest::join_threads(void) {
  // Ensure that all threads have completed so that none are
  // waiting for any state
  for (auto &t:_th)
    t.join();

  // clean up the prior invocation state in case one would like to 
  // spawn threads and compute again
  _future_sums.resize(0);
  _th.resize(0);

  return;
}

void PackagedTaskTest::disp_state(void) {
  std::cout << "Size " << _siz << ": Range " << _range << std::endl;

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
    PackagedTaskTest ptt(argc, argv);

    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    ptt.spawn_threads();
    uint32_t sum = ptt.compute_sum();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    std::chrono::milliseconds dur = 
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    ptt.join_threads();
    // ptt.disp_state();
    std::cout << "Computed Sum " << sum << " in " << dur.count() << " milli secs" << std::endl;
  }
  catch(const std::string &s) {
    std::cerr << "Exception caught: " << s << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return 0;
}
