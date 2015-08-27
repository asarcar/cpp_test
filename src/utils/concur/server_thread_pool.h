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

//! @file   server_thread_pool.h
//! @brief  ServerThreadPool: Pool of threads executing function objects
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_CONCUR_SERVER_THREAD_POOL_H_
#define _UTILS_CONCUR_SERVER_THREAD_POOL_H_

// C++ Standard Headers
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
#include "utils/concur/concur_block_q.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    ServerThreadPool
//! @brief    Pool of threads executing function objects
template <typename F>
class ServerThreadPool {
 public:
  // num_threads: when 0 relies on the system to pick a "good"
  // number of threads to be spawned.
  ServerThreadPool(uint32_t num_threads = 0);       
  ~ServerThreadPool(void)                               = default;
  ServerThreadPool(const ServerThreadPool&)             = delete;
  ServerThreadPool& operator=(const ServerThreadPool &) = delete;
  ServerThreadPool(ServerThreadPool&&)                  = delete;
  ServerThreadPool& operator=(ServerThreadPool &&)      = delete;

  // Interface used by users to submit task to ServerThreadPool
  void submit_task(F&& f) {
    _sync_msg_q.Push(std::move(f));
    return;
  }

  // Ensure that all threads have completed termination from system perspective 
  inline void join_threads(void) {
    for (auto &t:_th_pool) {
      DLOG(INFO) << "TH " << std::hex << t.get_id() << " joining main...";
      t.join();
    }
    return;
  }
 private:
  ConcurBlockQ<F>                           _sync_msg_q;
  std::vector<std::thread>                  _th_pool;
};

template <typename F> 
ServerThreadPool<F>::ServerThreadPool(uint32_t num_threads) {
  if (num_threads == 0) 
    num_threads = std::thread::hardware_concurrency();
  DLOG(INFO) << "Main TH " << std::hex << std::this_thread::get_id() 
             << ": ServerThreadPool: Threads in pool " << num_threads;
  
  for (int i=0; i<static_cast<int>(num_threads); i++) {
    std::thread t = 
        std::thread(    
            [this]()->void {
              while (true) {
                F f = this->_sync_msg_q.Pop();
                DLOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
                           << ": Fn object popped";
                // dummy event posted: signal terminate thread
                if (f.valid() == false) {
                  DLOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
                             << " received termination event: terminating!";
                  return;
                }
                f();
                DLOG(INFO) << "TH " << std::hex << std::this_thread::get_id() 
                           << ": function object invoked function operator";
              }
              return;
            });
    DLOG(INFO) << "TH " << std::hex 
               << t.get_id() << " created as ServerPool to process tasks";
    _th_pool.push_back(std::move(t));
  }
    
  return;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_SERVER_THREAD_POOL_H_
