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
#include "utils/concur/spin_lock.h"

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    lock_guard
//! @brief    RAII for lock with shared or exclusive semantics
template <typename Lock=SpinLock>
class LockGuard {
 public:
  explicit LockGuard(Lock& m): 
      m_(m) {m_.lock();}
  explicit LockGuard(Lock& m, LockMode mode): 
      m_(m) {m_.lock(mode);}
  ~LockGuard(void) {m_.unlock();}
  LockGuard(const LockGuard& o)             = delete;
  LockGuard& operator=(const LockGuard& o)  = delete;
  LockGuard(LockGuard&& o)                  = delete; 
  LockGuard& operator=(LockGuard&& o)       = delete;

 private:
  Lock&  m_;
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_LOCK_GUARD_H_
