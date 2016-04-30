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

//! @file   thread_pool.h
//! @brief  ThreadPool: Provides pool of worker threads executing functors.
//! @detail All operations are thread safe. The class guarantees that all
//!         worker threads are destroyed before the ThreadPool class is destroyed.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_CONCUR_THREAD_POOL_H_
#define _UTILS_CONCUR_THREAD_POOL_H_

// C++ Standard Headers
#include <functional>       // std::function
#include <iostream>         // std::cout
#include <thread>           // std::thread
#include <vector>           // std::vector
// C Standard Headers
// Google Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/concur/cb_mgr.h"
#include "utils/concur/concur_block_q.h"
#include "utils/ds/elist.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    ServerThreadPool
//! @brief    Pool of threads executing function objects
template <typename F = std::function<void(void)>>
class ThreadPool {
 public:
  // num_threads: when 0 relies on the system to pick a "good"
  // number of threads to be spawned.
  ThreadPool(int num_ths = 0);       
  ~ThreadPool(void);
  ThreadPool(const ThreadPool&)             = delete;
  ThreadPool& operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool&&)                  = delete;
  ThreadPool& operator=(ThreadPool &&)      = delete;

  // Interface used by users to submit task to ServerThreadPool
  inline void AddTask(F&& f) {
    F fn{std::move(f)};
    task_mgr_.Seal(&fn); 
    task_q_.Push(std::move(fn));
  }

 private:
  ConcurBlockQ<F>                           task_q_;
  asarcar::utils::ds::Elist<std::thread>    task_ths_;
  CbMgr<F>                                  task_mgr_;

  static void TaskFn(ThreadPool<F> *);
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_THREAD_POOL_H_
