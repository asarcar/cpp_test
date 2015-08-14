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

#ifndef _UTILS_CONCUR_RW_LOCK_GUARD_H_
#define _UTILS_CONCUR_RW_LOCK_GUARD_H_

//! @file   rw_lock_guard.h
//! @brief  rw_lock_guard: RAII for rw_lock
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <iostream>             // std::cout
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/concur/rw_mutex.h"

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    rw_lock_guard
//! @brief    RAII for rw_lock
//! Only rw_locking is supported.
//! Behavior undefined if rw_lock_guard is called recursively
//! Behavior undefined if rw_mutex is destroyed before rw_lock_guard.
class rw_lock_guard {
 public:
  explicit rw_lock_guard(rw_mutex &rwm, LockMode mode) : 
      _rwm(rwm), _mode{mode} {
    _rwm.lock(_mode);
  }
  ~rw_lock_guard(void) {_rwm.unlock();}
  rw_lock_guard(const rw_lock_guard& o)             = delete;
  rw_lock_guard& operator=(const rw_lock_guard& o)  = delete;
  rw_lock_guard(rw_lock_guard&& o)                  = delete; 
  rw_lock_guard& operator=(rw_lock_guard&& o)       = delete;

 private:
  rw_mutex&           _rwm;
  LockMode            _mode;
};    

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_RW_LOCK_GUARD_H_
