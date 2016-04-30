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
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/concur/thread_pool.h"

using namespace std;
using namespace ::asarcar::utils::ds;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename F>
void ThreadPool<F>::TaskFn(ThreadPool<F> *p) {
  int task_num=0;
  while (true) {
    F f = p->task_q_.Pop();
    // dummy event posted: signal terminate thread
    if (!f) {
      DLOG(INFO) << "TH " << hex << this_thread::get_id() 
                 << " received termination event after processing " 
                 << task_num << " events: terminating!";
      return;
    }
    ++task_num;
    f();
    DLOG(INFO) << "TH " << hex << this_thread::get_id() 
               << ": task_num " << dec << task_num << " invoked";
  }
  return;
}

template <typename F> 
ThreadPool<F>::ThreadPool(int num_threads) {
  num_threads =(num_threads <= 0)?thread::hardware_concurrency():num_threads;
  DLOG(INFO) << "Main TH " << hex << this_thread::get_id() 
             << ": ThreadPool: " << num_threads << " Threads in pool";
  
  for (int i=0; i<num_threads; i++) {
    task_ths_.emplace_back(thread(bind(&TaskFn, this)));
    DLOG(INFO) << "TH " << hex 
               << task_ths_.back().get_id() 
               << " created as ThreadPool to process tasks";
  }
  
  return;
}

template <typename F> 
ThreadPool<F>::~ThreadPool() {
  DLOG(INFO) << "Main TH " << hex << this_thread::get_id() 
             << ": Destroy ThreadPool: " 
             << dec << task_ths_.size() << " Threads";
  // Worker threads would be scavenged in the next few lines
  // Apart from workers, were there any other thread processing callbacks,
  // we ensure that all callback added via ThreadPool has a lifetime 
  // that is less than ThreadPool lifetime
  CB_QUASH_N_WAIT(task_mgr_);

  // Terminate Task Threads by sending a null functor
  for (int i=0; i<task_ths_.size(); ++i) 
    task_q_.Push(F{});
  
  for (auto &t:task_ths_) {
    thread::id id = t.get_id();
    t.join();
    DLOG(INFO) << "TH " << hex << id << " joined base thread "
               << this_thread::get_id();
  }
}

//-----------------------------------------------------------------------------
// Instantiate Templates for Void Function
template class ThreadPool<function<void(void)>>;

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
