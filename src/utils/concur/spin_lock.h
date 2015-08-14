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

#ifndef _UTILS_CONCUR_SPIN_LOCK_H_
#define _UTILS_CONCUR_SPIN_LOCK_H_

//! @file   spin_lock.h
//! @brief  Allows callers to spin if lock cannot be acquired immediately
//! @detail During mutex locking the OS may send the thread to sleep and 
//!         wake it up when the lock can be obtained by this thread. 
//!         Sending a thread to sleep and waking it are somewhat expensive 
//!         operations but under some circumstances the OS may replace the 
//!         waiting thread with another that has some meaningful work. 
//!         Spinlocks use another way of waiting called “busy waiting”. 
//!         Instead of sending the thread to sleep, spinlocks “trap” the 
//!         thread in a loop by constantly checking if the lock can be 
//!         obtained. During busy waiting the thread is kept alive making 
//!         it difficult for the OS to replace it with another one.
//!         Acquisition of the lock in NOT fair.
//!
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <atomic>       // std::atomic_flag
#include <iostream>     // std::ostream
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/concur/lock.h"

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
class SpinLock {
 public:
  // max # times we spin iterate before yielding to other threads
  static constexpr int MAX_SPIN_ITERATIONS = 100; 

  explicit SpinLock() {};
  ~SpinLock() = default;
  // Prevent bad usage: copy and assignment of SpinLock
  SpinLock(const SpinLock&)             = delete;
  SpinLock& operator =(const SpinLock&) = delete;
  SpinLock(SpinLock&&)                  = delete;
  SpinLock& operator =(SpinLock&&)      = delete;

  void lock(void);
  inline void unlock(void) {
    DCHECK_NE(mode_, LockMode::UNLOCK);
    mode_ = LockMode::UNLOCK;
    lck_.clear(std::memory_order_release); 
  }

  // Attempts to take the lock: return TRUE if successful, FALSE otherwise
  inline bool TryLock() {
    return !lck_.test_and_set(std::memory_order_acquire);
  }

  std::string to_string(void);
  friend std::ostream& operator<<(std::ostream& os, SpinLock& s);

 private:
  std::atomic_flag lck_{ATOMIC_FLAG_INIT};

  // Currently: Only two SpinLock modes supported: UNLOCK or EXCLUSIVE_LOCK
  // Since we assume spinlocks are taken for very short duration
  // benefit of SHARED_LOCK is not seen since we assume minimal contention
  LockMode         mode_{LockMode::UNLOCK}; 
  // # times we spun before success for the last lock acquired
  int              num_spins_{0};     
  // # threads waiting to acquire spinlock and busy waiting
  std::atomic<int> num_waiting_ths_{0};

  inline void UpdateLockStats(int num) {
    mode_      = LockMode::EXCLUSIVE_LOCK;
    num_spins_ = num;
  }
};

std::ostream& operator<<(std::ostream& os, SpinLock& s);

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_MONITOR_H_
