// Copyright 2015 asarcar Inc.
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

#ifndef _UTILS_CONCUR_LOCK_GUARD_H_
#define _UTILS_CONCUR_LOCK_GUARD_H_

//! @file   lock_guard.h
//! @brief  lock_guard: RAII wrapper for locks supporting SHARE/EXCLUSIVE modes
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <iostream>             // std::cout
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/concur/lock.h"

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    lock_guard
//! @brief    RAII for lock with shared or exclusive semantics
//! Behavior undefined if rw_lock_guard is called recursively
//! Behavior undefined if rw_mutex is destroyed before rw_lock_guard.
template<typename Mutex>
class lock_guard
{
 public:
  explicit lock_guard(Mutex& m) : 
      mutex_(m) { 
    mutex_.lock();
  }
  explicit lock_guard(Mutex& m, LockMode mode) : 
      mutex_(m) { 
    mutex_.lock(mode); 
  }
  ~lock_guard(void) {mutex_.unlock();}
  lock_guard(const lock_guard& o)             = delete;
  lock_guard& operator=(const lock_guard& o)  = delete;
  lock_guard(lock_guard&& o)                  = delete; 
  lock_guard& operator=(lock_guard&& o)       = delete;

 private:
  Mutex&              mutex_;
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_LOCK_GUARD_H_
