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
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/concur_block_q.h"
#include "utils/concur/server_thread_pool.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

static uint32_t parse_args(int argc, char *argv[]);

class MyFn {
 public:
  using MyFnRetType     = uint32_t;
  using MyFnType        = MyFnRetType(void);
  using PackageTaskType = std::packaged_task<MyFnType>;
  using FutureType      = std::future<MyFnRetType>;
  
  // C++11 wierd bug: vals{v} instead of vals(v) invokes copy ctor
  inline MyFn(const std::vector<uint32_t>& v, uint32_t f, uint32_t l) :
      _vals(v), _first{f}, _last{l} {};
  ~MyFn()                         = default;
  // C++11 wierd bug: vals{v} instead of vals(v) invokes copy ctor
  inline MyFn(MyFn &&o) : 
      _vals(o._vals), _first{o._first}, _last{o._last} {};
  MyFn(const MyFn& o)             = delete;
  MyFn& operator=(MyFn &&o)       = delete;
  MyFn& operator=(const MyFn& o)  = delete;
  inline uint32_t operator()(void) {
    uint32_t val = std::accumulate(_vals.begin() + _first, 
                                   _vals.begin() + _last, 0);
    DLOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
               << " SUM of vector " << std::hex << _vals.data() 
               << ": compute range [" << std::dec
               << _first  << "," << _last << ") = " << val;
    return val;
  }
  inline bool valid(void) {return (_first < _last);}
  friend std::ostream& operator<<(std::ostream &os, MyFn &f) {
    os << std::hex << "_vals.data() " << f._vals.data() 
       << std::dec << ": _first " << f._first << ": _last " << f._last
       << std::boolalpha << ": valid " << f.valid(); 
    return os;
  }
 private:
  const std::vector<uint32_t>&    _vals;
  uint32_t                        _first;
  uint32_t                        _last;
};

class ServerThreadPoolTest {
 public:
  constexpr static uint32_t DEF_SIZ = (1 << 12);
  explicit ServerThreadPoolTest(uint32_t num_vals); 

  ~ServerThreadPoolTest(void)                                     = default;
  ServerThreadPoolTest(const ServerThreadPoolTest &o)             = delete;
  ServerThreadPoolTest(ServerThreadPoolTest &&o)                  = delete;
  ServerThreadPoolTest& operator=(const ServerThreadPoolTest &o)  = delete;
  ServerThreadPoolTest& operator=(ServerThreadPoolTest &&o)       = delete;

  void submit_tasks(void);
  uint32_t compute_sum(void);
  void join_threads(void);
  void disp_state(void);

 private:
  // don't break into subtasks (multiple ranges) unless size >= 32K
  constexpr static uint32_t MIN_SIZ = (DEF_SIZ >> 1); 

  uint32_t                      _num_ths;
  ServerThreadPool<MyFn::PackageTaskType> _stp;
  uint32_t                      _siz     = DEF_SIZ;
  uint32_t                      _range   = 0;
  std::vector<uint32_t>         _vals    = {};
  std::vector<uint32_t>         _lims    = {};
  std::vector<MyFn::FutureType> _fu_pool = {};
  uint32_t                      _sum     = 0;

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

ServerThreadPoolTest::ServerThreadPoolTest(uint32_t num_vals) : 
    _num_ths{std::thread::hardware_concurrency()}, _stp{_num_ths}
{
  FASSERT(num_vals > 0);
  _siz = num_vals;
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

  DLOG(INFO) << "Vector " << std::hex << _vals.data() << std::dec
             << ": size " << _vals.size() 
             << ": broken num_ranges " << _range;

  return;
}

void ServerThreadPoolTest::submit_tasks(void) {
  // Remember future from each PackageTask and store in vector
  // so that the value can be extracted for computing sum
  for (int i=0; i<static_cast<int>(_range); ++i) {
    MyFn::PackageTaskType pt{MyFn{_vals, _lims[i], _lims[i+1]}};
    _fu_pool.push_back(pt.get_future());
    _stp.submit_task(std::move(pt));
  }

  // Now that all computation requests are inserted
  // create dummy computation terminate requests
  for (uint32_t i=0; i<_num_ths; ++i) {
    // Packaged Task: default ctor does not hold any task: no exception
    _stp.submit_task(MyFn::PackageTaskType{});
  }

  return;
}

uint32_t ServerThreadPoolTest::compute_sum(void) {
  _sum = 0;
  for (auto &fu : _fu_pool) {
    _sum += fu.get();
  }

  DLOG(INFO) << "MainThread TH " << std::hex << std::this_thread::get_id() 
             << ": num_ranges " << std::dec
             << _fu_pool.size() << ": sub-sum " << _sum;

  return _sum;
}

void ServerThreadPoolTest::join_threads(void) {
  _stp.join_threads();
  return;
}

void ServerThreadPoolTest::disp_state(void) {
  DLOG(INFO) << "Size " << _siz << ": Range " << _range 
             << ": NumThs " << _num_ths << std::endl;
  
  DLOG(INFO) << "Values: #elem " << _vals.size() << std::endl;
  uint32_t i=0;
  for (auto &val: _vals) {
    DLOG(INFO) << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      DLOG(INFO) << std::endl;
  }
  DLOG(INFO) << std::endl;
  
  DLOG(INFO) << "Range: #elem " << _lims.size() << std::endl;
  i=0;
  for (auto &val: _lims) {
    DLOG(INFO) << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      DLOG(INFO) << std::endl;
  }
  DLOG(INFO) << std::endl;
  
  DLOG(INFO) << "Total Sum " << _sum << std::endl;
  
  return;
}

class STPTest {
 public:
  static constexpr uint32_t NUM_VALS_INCREMENT = ServerThreadPoolTest::DEF_SIZ;
  static constexpr uint32_t NUM_SERVER_POOLS = 8;
  using SvrPtr = std::unique_ptr<ServerThreadPoolTest>;
  using SvrPtrArray = std::array<SvrPtr, NUM_SERVER_POOLS>;
  using ValArray = std::array<uint32_t, NUM_SERVER_POOLS>;

  explicit STPTest(uint32_t num_vals, bool auto_test);
  void exec_tests(void);

 private:
  bool _auto_test = false;
  SvrPtrArray _stpt_ptr_pool;
  ValArray _val_siz_pool;
};

STPTest::STPTest(uint32_t num_vals, bool auto_test) : _auto_test{auto_test} {
  uint32_t num_of_vals = num_vals;
  for (int i=0; i<static_cast<int>(NUM_SERVER_POOLS); ++i) {
    if ((_auto_test) && (i>=1)) 
      return;
    _stpt_ptr_pool.at(i).reset(new ServerThreadPoolTest{num_of_vals});
    _val_siz_pool.at(i) = num_of_vals;
    num_of_vals += NUM_VALS_INCREMENT;
  }
  return;
}

void STPTest::exec_tests(void) {
  for (int i=0; i<static_cast<int>(NUM_SERVER_POOLS); ++i) {
    if ((_auto_test) && (i>=1)) 
      return;
    std::chrono::time_point<std::chrono::system_clock> start = 
        std::chrono::system_clock::now();
    uint32_t num_vals = _val_siz_pool.at(i); 
    _stpt_ptr_pool.at(i)->submit_tasks();
    uint32_t sum = _stpt_ptr_pool.at(i)->compute_sum();
    std::chrono::time_point<std::chrono::system_clock> end = 
        std::chrono::system_clock::now();
    std::chrono::milliseconds dur = 
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    _stpt_ptr_pool.at(i)->join_threads();
    uint32_t exp_sum = (num_vals*(num_vals+1))/2;
    CHECK_EQ(sum, exp_sum) << "STPTest Failed: Index " << i 
                           << ": Num_Vals " << num_vals
                           << ": Computed Sum " << sum 
                           << "!= Expected Sum " << exp_sum;
    std::cout << "Computed Sum " << sum << " in " 
              << dur.count() << " milli secs" << std::endl;
    LOG(INFO) << "Computed Sum " << sum << " in " 
              << dur.count() << " milli secs";
  }
  return;
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  try {
    uint32_t num_vals = parse_args(argc, argv);
    LOG(INFO) << argv[0] << " called: argc " << argc << ": num_vals " << num_vals;

    STPTest stp(num_vals, FLAGS_auto_test);
    stp.exec_tests();
  }
  catch(const std::string &s) {
    LOG(WARNING) << "String Exception caught: " << s;
    return -1;
  }
  catch(std::exception &e) {
    LOG(WARNING) << "Exception caught: " << e.what();
    return -1;
  }
  catch(...) {
    LOG(WARNING) << "General Exception caught";
    return -1;
  }

  return 0;
}

uint32_t parse_args(int argc, char *argv[]) {
  // Parse Arguments...
  if (argc == 1)
    return ServerThreadPoolTest::DEF_SIZ;
  int s=0;
  if (argc >= 2)
    s = std::stoi(std::string(argv[1]));
  if ((s <= 0) || (argc > 2)) {
    std::ostringstream oss;
    oss << "Usage: " << argv[0] << " [arr_siz]" << std::endl;
    throw oss.str();
  }

  return s;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
