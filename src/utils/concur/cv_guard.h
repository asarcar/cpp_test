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

#ifndef _UTILS_CONCUR_COND_VAR_H_
#define _UTILS_CONCUR_COND_VAR_H_

//! @file   cv_guard.h
//! @brief  RAII for Condition Variable templatized on lock class.
//! @detail Condition variable that implementes Mesa (not Hoare)
//!         semantics on any lock. Assumptions & generic workflow:
//! //------------------------------------------------------------------------
//! Code Sample:
//! LockType lck;
//! CV<LockType,LockMode> cv{&lck};
//! //------------------------------------------------------------------------
//! // 1. WAITER(s) waits on condition variable while some condition is false
//! {
//!   CV::WaitGuard cv{cv, Predicate}; 
//!   // ... 
//!   // ... code executed when lock acquired and Predicate() is true
//!   // ... post code execution Predicate may or may not be updated
//! }
//! // Equivalent WAITER code explained below:
//! {
//!    lck.lock(mode); // mode == EXCLUSIVE_LOCK or SHARE_LOCK
//!    // Mesa (not Hoare) semantics requires testing in while loop
//!    // 1. If signaller acquires the lc first then waiters
//!    // may receive a receive a spurious wake signal i.e. 
//!    // although predicate not satisfied as signal message 
//!    // is made by signaller after unlocking lck.
//!    // while loop ensures that Mesa semantics works.
//!    // 2. The lock/unlock mutex around wait ensures that if waiter thread 
//!    // acquires lck first then it blocks and receives signal from signaller
//!    while(!Predicate()) { 
//!      cv.wait() & lck.unlock(); // atomic unlock & wait until cv signalled
//!      // lock *always* acquired in same mode. multiple readers may wait
//!      // on some condition and may be signalled by one writer
//!      lck.lock(lock_mode); // first acqurire lock on wakeing up
//!    }
//!    // Code executed when Predicate() is true - 
//!    // post-execution Predicate condition may be false
//!    lck.unlock();
//! }
//! //------------------------------------------------------------------------
//! // 2. SIGNALLER(s): signals on condition variable
//! {
//!   // broadcast (boolean): if true signal every thread waiting on lock
//!   CV::SignalGuard cv{lck, cv, broadcast}; 
//!   // ... code executed when lock acquired
//!   // ... post code execution Predicate must be true
//! }
//! // Equivalent SIGNALLER code explained below:
//! {
//!    lck.lock(EXCLUSIVE_LOCK);
//!    // Critical section code executed after which 
//!    // Predicate() above would start returning true
//!    lck.unlock(); 
//!    // We follow Mesa (not Hoare) semantics i.e. signal after releasing lock
//!    // also saves on context switches in case the waiter wakes up before
//!    // the signaller releases the lock
//!    if (notify_all) cv.notify_all(); else cv.notify_one(); 
//! }
//! //------------------------------------------------------------------------
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <atomic>       // std::atomic_flag
#include <functional>   // std::function
#include <mutex>        // std::mutex
#include <condition_variable>        // std::mutex
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/clock.h"
#include "utils/basic/meta.h"
#include "utils/concur/futex.h"
#include "utils/concur/lock.h"
#include "utils/concur/lock_guard.h"
#include "utils/concur/spin_lock.h"
//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

template <typename LockType=SpinLock, LockMode Mode=LockMode::EXCLUSIVE_LOCK>
class CV {
 public:
  explicit CV(LockType& lck): lck_(lck) {}
  ~CV(void)                   = default;
  CV(const CV& o)             = delete;
  CV& operator=(const CV& o)  = delete;
  CV(CV&& o)                  = delete; 
  CV& operator=(CV&& o)       = delete;
  inline void xlock(void) {lck_.lock();}
  template<LockMode nMode=Mode>
  inline EnableIf<nMode==LockMode::EXCLUSIVE_LOCK, void> lock(void) {xlock();}
  template<LockMode nMode=Mode>
  inline EnableIf<nMode==LockMode::SHARE_LOCK, void>lock(void){lck_.lock(Mode);}
  inline void unlock(void) {lck_.unlock();}

  class WaitGuard {
   public:
    friend class CV;
    // extra-args: 
    // wait_msecs: 0 when waiting not allowed: abort if predicate false
    //             MaxDuration() wait indefinitely until predicate true
    // success: false when pred failed and we aborted due to timeout
    template <typename Predicate>
    explicit WaitGuard(CV<LockType,Mode>& cv, Predicate pred, 
                       const Clock::TimeDuration wait_msecs, 
                       bool* success_p);

    template <typename Predicate>
    explicit WaitGuard(CV<LockType,Mode>& cv, Predicate pred) : 
        WaitGuard{cv, pred, Clock::MaxDuration(), nullptr} {}
 
    ~WaitGuard(void) { cv_.unlock(); }

   private:
    CV<LockType,Mode>& cv_;
  };
  
  class SignalGuard {
   public:
    friend class CV;
    template <LockMode nMode = Mode>
    explicit SignalGuard(CV<LockType,Mode>& cv, 
                         EnableIf<nMode==LockMode::SHARE_LOCK, bool> broadcast): 
        cv_(cv), broadcast_{broadcast} {cv_.xlock();} 
    explicit SignalGuard(CV<LockType,Mode>& cv): 
        cv_(cv), broadcast_{false} {cv_.xlock();}
    ~SignalGuard(void);
   private:
    CV<LockType,Mode>& cv_;
    bool      broadcast_;
  };

 private:
  LockType&               lck_;
  std::mutex              m_;
  std::condition_variable cond_;
};

template <typename LockType=SpinLock, LockMode Mode=LockMode::EXCLUSIVE_LOCK>
using CvWg = typename CV<LockType,Mode>::WaitGuard;

template <typename LockType=SpinLock, LockMode Mode=LockMode::EXCLUSIVE_LOCK>
using CvSg = typename CV<LockType,Mode>::SignalGuard;


template <typename LockType, LockMode Mode>
template <typename Predicate>
CV<LockType,Mode>::
WaitGuard::WaitGuard(CV<LockType,Mode>& cv, Predicate pred, 
                     const Clock::TimeDuration wait_msecs, 
                     bool* success_p) : cv_(cv) {
  Clock::TimePoint begin_msecs = Clock::MSecs();
  bool success; 
  success_p = (success_p == nullptr) ? &success : success_p;
  *success_p = false;

  cv_.lock();
  while(!pred()) { 
    // if wait time is bounded then quit if time exceeded 
    if (wait_msecs == 0 || (Clock::MSecs() - begin_msecs) > wait_msecs)
      return;
    // Atomic operation (enforced via mutex): wait on cond & release mutex
    { 
      std::unique_lock<std::mutex> lkg{cv_.m_}; 
      cv_.unlock(); 
      cv_.cond_.wait(lkg);
    }
    cv_.lock(); // wake signal - first acquire lock in same mode
  }

  *success_p = true;
  return;
}

template <typename LockType, LockMode Mode>
CV<LockType,Mode>::
SignalGuard::~SignalGuard(void) {
  { // Don't miss signal when waiter will wait
    LockGuard<std::mutex> lkg{cv_.m_};
  } 
  cv_.unlock(); // unlock and then signal
  if (broadcast_) { 
    cv_.cond_.notify_all(); 
    return; 
  }
  cv_.cond_.notify_one();
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_COND_VAR_H_
