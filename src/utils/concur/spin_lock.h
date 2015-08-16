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
  // All Memory Order Operations on ATOMICs are kept SEQUENTIALY CONSISTENT 
  // Otherwise it becomes very hard to reason
  static constexpr std::memory_order MEM_ORDER = std::memory_order_seq_cst;
  // max # times we spin iterate before yielding to other threads
  static constexpr int MAX_SPIN_ITERATIONS = 100; 
  // Lock State
  // 1. UNLOCK              0
  // 2. EXCLUSIVE_LOCK     -1  (<0)
  // 3. SHARE_LOCK         +X  with X objects owning shared lock
  //    SHARE_INC_LOCK_VAL  1  inc with each new share lock member
  static constexpr int UNLOCK_VAL            =  0;
  static constexpr int EXCLUSIVE_LOCK_VAL    = -1;
  static constexpr int SHARE_DELTA_LOCK_VAL  =  1;
  
  explicit SpinLock() {};
  ~SpinLock() = default;
  // Prevent bad usage: copy and assignment of SpinLock
  SpinLock(const SpinLock&)             = delete;
  SpinLock& operator =(const SpinLock&) = delete;
  SpinLock(SpinLock&&)                  = delete;
  SpinLock& operator =(SpinLock&&)      = delete;

  void lock(LockMode mode = LockMode::EXCLUSIVE_LOCK);
  void unlock(void);
  LockMode Mode(void); // snapshot of the current mode of the lock

  // Attempts to take the lock: return TRUE if successful, FALSE otherwise
  bool TryLock(LockMode mode = LockMode::EXCLUSIVE_LOCK);
  // Attempts to upgrade the lock from Share to Exclusive.
  // returns TRUE if successful, false otherwise
  bool TryUpgrade(void);
  // Attempts to downgrade the lock from Exclusive to Share
  // returns TRUE if successful, false otherwise
  void Downgrade(void);
  
  std::string to_string(void);
  friend std::ostream& operator<<(std::ostream& os, SpinLock& s);

 private:
  // lock state values: UNLOCK, EXCLUSIVE_LOCK, or SHARE_LOCK
  std::atomic_int val_{0};


  // debug stats: # times we spun before success for the last lock acquired
  std::atomic_int  num_spins_{0};     
  // debug stats: # threads waiting to acquire spinlock and busy waiting
  std::atomic_int  num_waiting_ths_{0};

  inline void UpdateLockStats(int num) { 
    // Comman separated expression returns the value of the last expression:
    // (a, b) returns b. We used this method to ensure num_spins_ (debug stats)
    // is only updated when NDEBUG is turned on.
    DCHECK_GT((num_spins_.store(num), num), 0); 
  }
};

std::ostream& operator<<(std::ostream& os, SpinLock& s);

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_MONITOR_H_
