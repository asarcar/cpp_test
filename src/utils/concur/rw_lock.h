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

#ifndef _UTILS_CONCUR_RW_LOCK_H_
#define _UTILS_CONCUR_RW_LOCK_H_

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
#include "utils/concur/cv_guard.h"
#include "utils/concur/lock.h"
#include "utils/concur/spin_lock.h"
#include "utils/ds/elist.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

namespace ds = ::asarcar::utils::ds;

// Forward Declaration
template <typename LockType>
class RWLock;
template <typename LockType>
std::ostream& operator<<(std::ostream&os, const RWLock<LockType>& rwm);

//! @class    RWLock
//! @brief    Supports multiple readers and single writer
//! behavior undefined if rw_lock is called recursively 
template <typename LockType = SpinLock>
class RWLock {
 public:
  using QElem = std::pair<std::thread::id, LockMode>;
  using QIterator = std::deque<QElem>::iterator;

  explicit RWLock(void) : lck_{}, cv_{lck_}, owners_{}, 
    q_pending_{}, cur_mode_{LockMode::UNLOCK}, num_readers_{0} {}
  ~RWLock(void)                       = default;
  RWLock(const RWLock& o)            = delete;
  RWLock& operator=(const RWLock& o) = delete;
  RWLock(RWLock&& o)                 = delete; 
  RWLock& operator=(RWLock&& o)      = delete;

  void lock(LockMode mode = LockMode::EXCLUSIVE_LOCK);
  void unlock(void);
  friend std::ostream& operator<< <>(std::ostream& os, const RWLock& rwm);

 private:
  LockType                      lck_;
  CV<LockType>                  cv_;
  ds::Elist<std::thread::id>    owners_;
  // deque is implemented as a chunks of blocks/arrays
  std::deque<QElem>             q_pending_; 
  LockMode                      cur_mode_;
  int                           num_readers_;
};    

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_RW_LOCK_H_
