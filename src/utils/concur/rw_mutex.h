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

#ifndef _UTILS_CONCUR_RW_MUTEX_H_
#define _UTILS_CONCUR_RW_MUTEX_H_

//! @file   rw_mutex.h
//! @brief  rw_mutex: Allows multiple readers and exclusive writer.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <deque>                // std::deque
#include <condition_variable>   // std::condition_variable
#include <iostream>             // std::cout
#include <mutex>                // std::mutex
#include <thread>               // std::thread
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/concur/lock.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    RWMutex
//! @brief    Supports multiple readers and single writer
//! behavior undefined if rw_lock is called recursively 
class RWMutex {
 public:
  using QElem = std::pair<std::thread::id, LockMode>;
  using QIterator = std::deque<QElem>::iterator;

  RWMutex(void)                        = default;
  ~RWMutex(void)                       = default;
  RWMutex(const RWMutex& o)            = delete;
  RWMutex& operator=(const RWMutex& o) = delete;
  RWMutex(RWMutex&& o)                 = delete; 
  RWMutex& operator=(RWMutex&& o)      = delete;

  void lock(LockMode mode = LockMode::EXCLUSIVE_LOCK);
  void unlock(void);
  friend std::ostream& operator<<(std::ostream& os, const RWMutex& rwm);

 private:
  std::condition_variable       cv_{};
  std::mutex                    mutex_{};
  // vector of all threads owning this lock
  // one may argue this should be a list for 
  // each insertion and deletion. benchmark shows that
  // vector exhibits better performance due to memory locality 
  // for small (<10K) number of elements
  std::vector<std::thread::id>  v_owners_{};
  // deque is implemented as a chunks of blocks/arrays
  std::deque<QElem>             q_pending_{}; 
  LockMode                      cur_mode_{LockMode::UNLOCK};
  int                           num_readers_{0};
};    

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_RW_MUTEX_H_
