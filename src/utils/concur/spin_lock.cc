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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <sstream>      // ostringstream
#include <thread>
// Standard C Headers
#include <sched.h>
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
  int curval = val_;
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
  constexpr int kMaxSpins = 1000;
  int num_iter;

  // Try to acquire the spinlock by trying kMaxSpin times 
  for (num_iter=0; num_iter < kMaxSpins; ++num_iter) {
    if (TryLock(mode)) {
      UpdateLockStats(num_iter+1);
      return;
    }

    // For busy waiting: Use 'pause' instruction to prevent speculative 
    // execution.
    // Reason: When we finally acquire the lock, we take the mother of all 
    // mispredicted branches leaving the while loop, which is the worst 
    // possible time for such a thing as you want duration one is holding
    // spinlocks to be very short. So post spin lock delays are deadly
    __asm volatile ("pause" ::: "memory");
  }

  // SCENARIO: 
  // We couldn't acquire spin lock: someone is holding the lock
  // for a long time. Let us yield to some other thread and try again.
  // yield method requires the calling thread to relinquish use of 
  // its processor IF any other thread of same priority is waiting 
  // in the run queue before it is scheduled again. 
  this_thread::yield();

  // We could not acquire the lock in tight loop: 
  // When many threads wait on the same lock
  // the performance quickly deteoriates due to "lock swarming" - 
  // cache snoop protocols wastes a lot of time. In such, conditions
  // better to back off to wait based signaling.
  for (;!TryLock(mode);++num_iter) {
    // Wait on futex is to be executed carefully. Do so only when we are 
    // *guaranteed* to lose lock request when wait is called.
    // Otherwise, it is possible the condition preventing acquisition of 
    // lock was cleared by the time wait is called. If so, we 
    // will be forever stuck on the WAIT loop, and the WAKE condition
    // will NEVER be executed.
    int saved = val_;
    // Exclusive or Shared Lock should succeed if val_ is UNLOCK_VAL
    if (saved == UNLOCK_VAL)
      continue;
    // ExclusiveLock will fail if val_ == saved: safe to call wait
    // SharedLock will fail if val_ == EXCLUSIVE_LOCK_VAL: call wait
    if (mode == LockMode::SHARE_LOCK)
      saved = EXCLUSIVE_LOCK_VAL;
    // Shared Lock would only fail if val_ was EXCLUSIVE_LOCK_VAL
    // debug accounting stats: we can be relaxed about operation
    f_.Wait(saved);
  }

  // lock aquired - update counter stats and return
  UpdateLockStats(num_iter + 1);

  return;
}

void SpinLock::unlock(void) {
  // Exclusive lock case?
  int exc_lck_val = EXCLUSIVE_LOCK_VAL; // expected value if we owned an EXCLUSIVE_LOCK
  bool exc_lock   = val_.compare_exchange_strong(exc_lck_val, 0, MEM_ORDER);

  // Shared lock case?
  if (!exc_lock) {
    DCHECK_GT(exc_lck_val, 0);
    // Assuming unlock state is not unpaired due to SW bug we are
    // guaranteed that the LOCK is in SHARE_LOCK state (val_ > 0) at 
    // this juncture. Atomically decrement irrespective of what other threads
    // are doing. 
    val_ -= SHARE_DELTA_LOCK_VAL; 
  }

  // signal waiting threads to compete for spinlock. 
  // Ideally: signal only one if threads waiting for EXCLUSICE lock only
  // Otherwise: signal FIFO way threads waiting for SHARED/EXCLUSIVE locks
  // Since: we are implementing a simple spinlock we just wake everyone
  // and let them fight it out
  PCHECK(f_.Wake(true) >= 0);

  return;
}

LockMode SpinLock::Mode(void) {
  int val = val_;
  return ((val == 0) ? LockMode::UNLOCK : 
          ((val > 0) ? LockMode::SHARE_LOCK : 
           LockMode::EXCLUSIVE_LOCK));
}

bool SpinLock::TryUpgrade(void) {
  DCHECK_GE(val_, SHARE_DELTA_LOCK_VAL);
  int val = SHARE_DELTA_LOCK_VAL;
  return val_.compare_exchange_weak(val, EXCLUSIVE_LOCK_VAL, MEM_ORDER);
} 

void SpinLock::Downgrade(void) {
  DCHECK_EQ(val_, EXCLUSIVE_LOCK_VAL);
  int val = EXCLUSIVE_LOCK_VAL;
  val_.compare_exchange_strong(val, SHARE_DELTA_LOCK_VAL, MEM_ORDER);

  // signal waiting threads to compete for spinlock. 
  // Ideally: signal only one if threads waiting for EXCLUSICE lock only
  // Otherwise: signal FIFO way threads waiting for SHARED/EXCLUSIVE locks
  // Since: we are implementing a simple spinlock we just wake everyone
  // and let them fight it out
  PCHECK(f_.Wake(true) >= 0);

  return;
}

string SpinLock::to_string(void) {
  ostringstream oss;
  oss << "SpinLock"
      << ": LockMode=" << Lock::to_string(Mode())
      << ": Debug Stats {num_spins=" << num_spins_ 
      << ",futex=" << f_.to_string() << "}";
  return oss.str();
}

ostream& operator<<(ostream& os, SpinLock& s) {
  os << s.to_string();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
