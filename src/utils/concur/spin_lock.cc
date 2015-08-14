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

void SpinLock::lock(void) {
  int num_iter, num = 0;

  if (TryLock()) {
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
      if (TryLock()) {
        UpdateLockStats(num + num_iter + 1);
        return;
      }
    }

    num += num_iter;

    num_waiting_ths_.fetch_add(1);

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
    num_waiting_ths_.fetch_sub(1);

    DCHECK_GE(num_waiting_ths_.load(), 0);
  }
  return;
}

string SpinLock::to_string(void) {
  return string("SpinLock: Mode ") + Lock::to_string(mode_) + 
      string(": num_spins ") + std::to_string(num_spins_) + 
      string(": num_waiting_ths ") + std::to_string(num_waiting_ths_.load());
}

ostream& operator<<(ostream& os, SpinLock& s) {
  os << s.to_string();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
