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

class MutexTest {
 public:
  explicit MutexTest(int argc, char **argv); 
  ~MutexTest() = default;

  MutexTest(const MutexTest &at) = delete;
  MutexTest(MutexTest &&at) = delete;
  MutexTest& operator=(const MutexTest &at) = delete;
  MutexTest& operator=(MutexTest &&at) = delete;

  void spawn_threads(void);
  uint32_t join_threads(void);
  void disp_state(void);
 private:
  const static uint32_t DEF_SIZ = 128;
  const static uint32_t MIN_SIZ = 32; // don't parallelize unless size > 32

  uint32_t _sum = 0;
  uint32_t _siz = DEF_SIZ;
  uint32_t _range = 0;

  std::mutex                         _m;
  std::vector<uint32_t>              _vals;
  std::vector<uint32_t>              _lims;
  std::vector<std::thread>           _th;

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

MutexTest:: MutexTest(int argc, char **argv) {
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

void MutexTest::spawn_threads(void) {
  // should not be called in succession
  assert(_sum == 0);
  assert(_th.size() == 0);

  // Set up the future array and spawn the async threads
  for (uint32_t j=0; j<_range; ++j) {
    // Spawn the Thread
    std::thread t([this, j]() {
        uint32_t val = std::accumulate(this->_vals.begin()+
                                       this->_lims.at(j), 
                                       this->_vals.begin()+
                                       this->_lims.at(j+1), 
                                       0);
        std::lock_guard<std::mutex> lck{this->_m};
        this->_sum += val;
        // unlock autmatically called via lock_guard destructor
        return;
      });
    _th.push_back(std::move(t));
  }
  
  return;
}

uint32_t MutexTest::join_threads(void) {
  // Ensure that all threads have completed so that none are
  // waiting for any state
  for (auto &t:_th)
    t.join();

  // clean up the prior invocation state in case one would like to 
  // spawn threads and compute again
  _th.resize(0);

  return _sum;
}


void MutexTest::disp_state(void) {
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
    MutexTest mt(argc, argv);
    
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    mt.spawn_threads();
    // unfortunately: we have not cleared a mechanism to signal all computation is done
    // hence we need to wait for all threads to update sum, terminate, and join main thread
    // to know the sum computation is complete
    uint32_t sum = mt.join_threads();
    std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    std::chrono::milliseconds dur = 
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // mt.disp_state();
    std::cout << "Computed Sum " << sum << " in " << dur.count() << " milli secs" << std::endl;
  }
  catch(const std::string &s) {
    std::cerr << "Exception caught: " << s << std::endl;
    std::exit(EXIT_FAILURE);
  }
  catch(...) {
    std::cerr << "Exception caught: " << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return 0;
}
