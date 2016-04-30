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
#include <array>       // std::array
#include <atomic>      // std::atomic_int
#include <future>      // std::packaged_task std::future
#include <iostream>
#include <numeric>     // std::accumulate
#include <sstream>     // std::ostringstream
#include <thread>      // std::thread
#include <vector>      // std::vector
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
#include "utils/concur/cv_guard.h"
#include "utils/concur/spin_lock.h"
#include "utils/concur/thread_pool.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

static int ParseArgs(int argc, char *argv[]);
using atomic_res = std::atomic<uint64_t>;

class MyFn {
 public:
  // C++11 wierd bug: vals{v} instead of vals(v) invokes copy ctor
  explicit MyFn(const vector<int>& v, int f, int l, 
                atomic_res* res_p, 
                atomic_int* ntask_p, CV<SpinLock>* cv_p) :
      vals_(v), first_{f}, last_{l}, res_p_{res_p}, 
    ntask_p_{ntask_p}, cv_p_{cv_p} {};
  ~MyFn()                         = default;
  // C++11 wierd bug: vals{v} instead of vals(v) invokes copy ctor
  inline MyFn(MyFn &&o) : 
      vals_(o.vals_), first_{o.first_}, 
    last_{o.last_}, res_p_{o.res_p_},
    ntask_p_{o.ntask_p_}, cv_p_{o.cv_p_} {};
  MyFn(const MyFn& o)             = default;
  MyFn& operator=(MyFn &&o)       = delete;
  MyFn& operator=(const MyFn& o)  = delete;
  inline void operator()(void) {
    DLOG(INFO) << "TH " << hex << this_thread::get_id() 
               << " Called " << __FUNCTION__;
    int val = accumulate(vals_.begin() + first_, 
                                   vals_.begin() + last_, 0);
    *res_p_ += val;
    *ntask_p_ -= 1;
    DLOG(INFO) << "TH " << hex << this_thread::get_id() 
               << " SUM of vector " << hex << vals_.data() 
               << ": compute range [" << dec
               << first_  << "," << last_ << ") = " 
               << ": val " << val << ": result " << *res_p_
               << ": ntask " << *ntask_p_;
    if (*ntask_p_ <= 0) {
      CV<SpinLock>::SignalGuard{*cv_p_};
    }
  }
  inline operator bool() {return (first_ < last_);}
  friend ostream& operator<<(ostream &os, MyFn &f) {
    os << hex << "_vals.data() " << f.vals_.data() 
       << dec << ": first " << f.first_ << ": last_ " << f.last_
       << boolalpha << ": empty " << !f
       << dec << ": result " << *f.res_p_;
    return os;
  }
 private:
  const vector<int>&    vals_;
  int                   first_;
  int                   last_;
  atomic_res*           res_p_;            
  atomic_int*           ntask_p_;
  CV<SpinLock>*         cv_p_;
};

class ThreadPoolTest {
 public:
  static constexpr int DEF_SIZ = (1 << 12);
  static constexpr int NUM_THREADS = 4;
  explicit ThreadPoolTest(int num_ths, int num_vals, atomic_res *res_p); 
  ~ThreadPoolTest(void)                               = default;
  ThreadPoolTest(const ThreadPoolTest &o)             = delete;
  ThreadPoolTest(ThreadPoolTest &&o)                  = delete;
  ThreadPoolTest& operator=(const ThreadPoolTest &o)  = delete;
  ThreadPoolTest& operator=(ThreadPoolTest &&o)       = delete;

  void AddTasks(void);
  void ComputeSum(void);
  void DispState(void);

 private:
  // don't break into subtasks (multiple ranges) unless size >= 32K
  constexpr static int MIN_SIZ = (DEF_SIZ >> 1); 

  int                               siz_{DEF_SIZ};
  int                               range_{0};
  vector<int>                       vals_{};
  vector<int>                       lims_{};
  atomic_int                        ntask_{0};
  SpinLock                          sl_{};
  CV<SpinLock>                      cv_{sl_};
  int                               num_ths_;
  atomic_res*                       res_p_;
  // ThreadPool declared last to destroy first
  ThreadPool<function<void(void)>>  tp_; 


  constexpr static inline int 
  get_num_ranges(int siz) {return (siz+MIN_SIZ-1)/MIN_SIZ;}

  // Initialize the ranges each task will process
  void init_limits(void);
};

ThreadPoolTest::ThreadPoolTest(int num_ths, int num_vals, atomic_res* res_p) : 
    num_ths_{num_ths}, res_p_{res_p}, tp_{num_ths_}
{
  FASSERT(num_vals > 0);
  siz_ = num_vals;
  // Break the array into elements of 32 elements each
  range_ = get_num_ranges(siz_);

  // Initialize Vectors...
  // Value Vector
  vals_.resize(siz_);
  int i=0;
  for (auto &val: vals_)
    val = ++i;
  // Limit Vector: reserve one additional entry needed for the last range
  lims_.resize(range_ + 1);
  init_limits();

  DLOG(INFO) << "Vector " << hex << vals_.data() << dec
             << ": size " << vals_.size() 
             << ": broken num_ranges " << range_;

  return;
}

void ThreadPoolTest::AddTasks(void) {
  int num_ths = (range_ <= NUM_THREADS) ? 0 : NUM_THREADS-1;
  DLOG(INFO) << "AddTasks: num_ths=" << num_ths << ": range_=" << range_;
             
  ntask_ = range_;
  // Use main thread as one thread to feed tasks to threadpool 
  for (int i=0; i<range_; i += num_ths+1)  {
    DLOG(INFO) << "AddTask: i=" << i << ": vals_.size()=" << vals_.size() 
               << ": lims=[" << lims_[i] << "," << lims_[i+1] << ")"
               << ": res_p_=" << hex << res_p_;
    tp_.AddTask(MyFn{vals_, lims_[i], lims_[i+1], res_p_, &ntask_, &cv_});
  }

  if (num_ths == 0)
    return;

  // create additional num_ths to feed the tasks to threadpool asap
  vector<thread> ths;
  for (int k=1; k<num_ths+1; ++k)
    ths.push_back(thread([this,k,num_ths](){
          for (int i=k; i<range_; i+=num_ths+1)
            tp_.AddTask(MyFn{vals_, lims_[i], lims_[i+1], res_p_, &ntask_, &cv_});
        }));

  for (int i=0; i<num_ths; ++i)
    ths.at(i).join();

  return;
}

void ThreadPoolTest::ComputeSum(void) {
  CV<SpinLock>::WaitGuard{cv_, [this](){return ntask_<=0;}};
  DLOG(INFO) << "Thread " << hex << this_thread::get_id()
             << ": ntask " << ntask_;
}

void ThreadPoolTest::DispState(void) {
  DLOG(INFO) << "Size " << siz_ << ": Range " << range_ 
             << ": NumThs " << num_ths_ << endl;
  
  DLOG(INFO) << "Values: #elem " << vals_.size() << endl;
  int i=0;
  for (auto &val: vals_) {
    DLOG(INFO) << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      DLOG(INFO) << endl;
  }
  DLOG(INFO) << endl;
  
  DLOG(INFO) << "Range: #elem " << lims_.size() << endl;
  i=0;
  for (auto &val: lims_) {
    DLOG(INFO) << ": [" << i++ << "]" << val;
    if ((i+1)%8 == 0)
      DLOG(INFO) << endl;
  }
  DLOG(INFO) << endl;
  
  DLOG(INFO) << "Result " << *res_p_ << endl;
  
  return;
}

void
ThreadPoolTest::init_limits(void) {
    int last_range=0;
    for (auto &lim:lims_) {
      lim = last_range; 
      last_range += MIN_SIZ;
    }
    // last range cannot exceed siz
    if (lims_.at(range_) > siz_)
      lims_.at(range_) = siz_;

    return;
}

class TPTest {
 public:
  static constexpr int NUM_VALS_MULT = 128;
  static constexpr int NUM_THS_MULT  = 4;
  static constexpr int NUM_THREAD_POOLS = 2;
  using Ptr          = unique_ptr<ThreadPoolTest>;
  using PtrArray     = array<Ptr, NUM_THREAD_POOLS>;
  using IntArray     = array<int, NUM_THREAD_POOLS>;
  using ResArray     = array<atomic_res, NUM_THREAD_POOLS>;
  using NumTaskArray = array<atomic_int, NUM_THREAD_POOLS>;

  explicit TPTest(int num_vals, bool auto_test);
  void ExecBasicClosureTests(void);
  void ExecPackagedTaskTest(void);
 private:
  bool         auto_test_{false};
  IntArray     num_th_pool_;
  IntArray     val_siz_pool_;
  ResArray     res_pool_;
  PtrArray     tpt_ptr_pool_;
};

TPTest::TPTest(int num_vals, bool auto_test) : 
    auto_test_{auto_test} {
  int num_ths = thread::hardware_concurrency();
  for (int i=0; i<NUM_THREAD_POOLS; ++i) {
    if ((auto_test_) && (i>=1)) 
      return;
    num_th_pool_.at(i)  = num_ths;
    val_siz_pool_.at(i) = num_vals;
    res_pool_.at(i)     = 0;
    tpt_ptr_pool_.at(i).
        reset(new ThreadPoolTest{num_ths, num_vals, &res_pool_.at(i)});
    num_ths             *= NUM_THS_MULT;
    num_vals            *= NUM_VALS_MULT;
  }
  return;
}

void TPTest::ExecBasicClosureTests(void) {
  for (int i=0; i<NUM_THREAD_POOLS; ++i) {
    if ((auto_test_) && (i>=1)) 
      return;
    Clock::TimePoint start = Clock::USecs();
    uint64_t num_vals = val_siz_pool_.at(i); 
    tpt_ptr_pool_.at(i)->AddTasks();
    tpt_ptr_pool_.at(i)->ComputeSum();
    uint64_t sum = res_pool_.at(i);
    Clock::TimeDuration dur = Clock::USecs() - start;
    uint64_t exp_sum = (num_vals*(num_vals+1))/2;
    CHECK_EQ(sum, exp_sum) << "ThreadPoolTest Failed: Index " << i 
                           << ": Num_Vals " << num_vals
                           << ": &res_pool_.at(i)=" << &res_pool_.at(i)
                           << ": Computed Sum " << sum 
                           << "!= Expected Sum " << exp_sum;
    LOG(INFO) << "Computed Sum " << dec << sum
              << ": for 1.." << num_vals
              << " in " << dur << " micro secs";
    // Destroy ThreadPool - Destructor waits till completion of all tasks
    tpt_ptr_pool_.at(i).reset();
  }

  return;
}

void TPTest::ExecPackagedTaskTest(void) {
  using Closure = function<int(void)>;
  using PT      = packaged_task<int(void)>;
  using Fut     = future<int>;

  constexpr int kNum = 2;

  ThreadPool<> tp;
  atomic_int count{0};
  Closure fn = [&count](){return ++count;};
  array<PT,kNum> pts{PT{fn}, PT{fn}};
  array<Fut, kNum> fus;

  for (int i=0; i<kNum; ++i) {
    PT *pt_p = &pts.at(i);
    fus.at(i) = pt_p->get_future();
    tp.AddTask([pt_p](){(*pt_p)();});
  }

  int val=0;
  for (int i=0; i<kNum; ++i)
    val += fus.at(i).get();

  CHECK_EQ(val, 3);
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  try {
    int num_vals = ParseArgs(argc, argv);
    LOG(INFO) << argv[0] << " called: argc " << argc << ": num_vals " << num_vals;
    {
      TPTest tpt(num_vals, FLAGS_auto_test);
      tpt.ExecBasicClosureTests();
      tpt.ExecPackagedTaskTest();
    }
  }
  catch(const string &s) {
    LOG(WARNING) << "String Exception caught: " << s;
    return -1;
  }
  catch(exception &e) {
    LOG(WARNING) << "Exception caught: " << e.what();
    return -1;
  }
  catch(...) {
    LOG(WARNING) << "General Exception caught";
    return -1;
  }

  return 0;
}

int ParseArgs(int argc, char *argv[]) {
  // Parse Arguments...
  if (argc == 1)
    return ThreadPoolTest::DEF_SIZ;
  int s=0;
  if (argc >= 2)
    s = stoi(string(argv[1]));
  if ((s <= 0) || (argc > 2)) {
    ostringstream oss;
    oss << "Usage: " << argv[0] << " [arr_siz]" << endl;
    throw oss.str();
  }

  return s;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
