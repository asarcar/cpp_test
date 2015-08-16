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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <thread>
// Standard C Headers
// Google Headers
// Local Headers
#include "utils/concur/spin_lock.h"

using namespace std;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

constexpr int SpinLock::MAX_SPIN_ITERATIONS;
constexpr int SpinLock::UNLOCK_VAL;
constexpr int SpinLock::EXCLUSIVE_LOCK_VAL;
constexpr int SpinLock::SHARE_DELTA_LOCK_VAL;

constexpr std::memory_order SpinLock::MEM_ORDER;

// Weak but efficient attempt to acquire lock. 
// Typically Succeeds when there is no contention on lock
// and the lock is readily available.
bool SpinLock::TryLock(LockMode mode) {
  DCHECK_NE(mode, LockMode::UNLOCK);

  // Currently: LOCK EXCLUSIVELY acquired by someone else: fail immediately
  int curval = val_.load();
  if (curval == EXCLUSIVE_LOCK_VAL)
    return false;

  // SHARE_LOCK case: Increment current value of val_ which is guaranteed >=0 
  if (mode == LockMode::SHARE_LOCK) 
    return val_.compare_exchange_weak(curval, 
                                      curval+SHARE_DELTA_LOCK_VAL,
                                      MEM_ORDER);

  // EXCLUSIVE_LOCK case: honored when val_ has UNLOCK_VAL 
  int expval = UNLOCK_VAL;
  return val_.compare_exchange_weak(expval, EXCLUSIVE_LOCK_VAL, MEM_ORDER);
}

void SpinLock::lock(LockMode mode) {
  int num_iter, num = 0;

  if (TryLock(mode)) {
    UpdateLockStats(1);
    return;
  }

  while (true) {
    for (num_iter=0; num_iter < MAX_SPIN_ITERATIONS; ++num_iter) {
      // For busy waiting: Use 'pause' instruction to prevent speculative 
      // execution.
      // Reason: When we finally acquire the lock, we take the mother of all 
      // mispredicted branches leaving the while loop, which is the worst 
      // possible time for such a thing as you want duration one is holding
      // spinlocks to be very short. So post spin lock delays are deadly
      __asm volatile ("pause" ::: "memory");

      // lock aquired? update counter stats and return
      if (TryLock(mode)) {
        UpdateLockStats(num + num_iter + 1);
        return;
      }
    }

    num += num_iter;
 
    // accounting stats: we can be relaxed about operation
    num_waiting_ths_.fetch_add(1, memory_order_relaxed); 

    // SCENARIO: 
    // We couldn't acquire spin lock: someone is holding the lock
    // for a long time. Let us yield to some other thread and try again.
    // 1. yield method requires the calling thread to relinquish use of 
    //    its processor IF any other thread of same priority is waiting 
    //    in the run queue before it is scheduled again. 
    // 2. sleep() makes the calling process sleep until seconds
    //    have elapsed or a signal arrives which is not ignored.
    this_thread::yield();

    // We could have added this thread on a FUTEX waiting on the value
    // of the atomic value to change. However, the primary reason 
    // for SpinLock is to busy wait and eliminate the context
    // switch. Hence we are not using that option here.
    // accounting stats: we can be relaxed about operation
    num_waiting_ths_.fetch_sub(1, memory_order_relaxed);

    DCHECK_GE(num_waiting_ths_.load(), 0);
  }
  return;
}

void SpinLock::unlock(void) {
  // Exclusive lock case:
  int exc_lck_val = EXCLUSIVE_LOCK_VAL; // expected value if we owned an EXCLUSIVE_LOCK
  if (val_.compare_exchange_strong(exc_lck_val, 0, MEM_ORDER))
    return;
  

  // Shared lock case:
  DCHECK_GT(exc_lck_val, 0);

  // Assuming unlock state is not unpaired due to SW bug we 
  // are guaranteed that the LOCK is in SHARE_LOCK state (val_ > 0) at 
  // this juncture. Atomically decrement irrespective of what other threads
  // are doing.
  val_.fetch_sub(SHARE_DELTA_LOCK_VAL, MEM_ORDER); 

  return;
}

LockMode SpinLock::Mode(void) {
  int val = val_.load();
  return ((val == 0) ? LockMode::UNLOCK : 
          ((val > 0) ? LockMode::SHARE_LOCK : 
           LockMode::EXCLUSIVE_LOCK));
}

bool SpinLock::TryUpgrade(void) {
  DCHECK_GE(val_.load(), SHARE_DELTA_LOCK_VAL);
  int val = SHARE_DELTA_LOCK_VAL;
  return val_.compare_exchange_weak(val, EXCLUSIVE_LOCK_VAL, MEM_ORDER);
} 

void SpinLock::Downgrade(void) {
  DCHECK_EQ(val_.load(), EXCLUSIVE_LOCK_VAL);
  int val = EXCLUSIVE_LOCK_VAL;
  val_.compare_exchange_strong(val, SHARE_DELTA_LOCK_VAL, MEM_ORDER);
  return;
}

string SpinLock::to_string(void) {
  return string("SpinLock: Value ") + std::to_string(val_.load())
      + string(": LockMode ") + 
      Lock::to_string(Mode()) +
      string(": num_spins ") + std::to_string(num_spins_.load()) + 
      string(": num_waiting_ths ") + std::to_string(num_waiting_ths_.load());
}

ostream& operator<<(ostream& os, SpinLock& s) {
  os << s.to_string();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
