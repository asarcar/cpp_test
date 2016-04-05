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

#ifndef _UTILS_CONCUR_FUTEX_H_
#define _UTILS_CONCUR_FUTEX_H_

//! @file   futex.h
//! @brief  wrapper over futex - fast synchronization primitives
//! @detail Futex wrapper allows userspace code to efficiently implement 
//!         synchronization primitives and thus simplify the job of 
//!         exploiting parallelism. 
//!         "Fast Userspace Mutexes" - original goal was to implement 
//!         the mutex primitive. Over time it has expanded into a generic 
//!         interface to allow the manipulation of wait-queues of 
//!         sleeping threads and processes.
//!
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <atomic>       // std::atomic_int
#include <limits>       // std::numeric_limits<T>::max()
// Standard C Headers
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
// Google Headers
#include <glog/logging.h>   
// Local Headers

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

class Futex {
 public:
  Futex(std::atomic_int *val_p) : 
      val_p_{val_p}, num_{0} {DCHECK(val_p != nullptr);}
  ~Futex() = default;
  // Prevent bad usage: copy and assignment of Futex
  Futex(const Futex&)             = delete;
  Futex& operator =(const Futex&) = delete;
  Futex(Futex&&)                  = delete;
  Futex& operator =(Futex&&)      = delete;

  //! @brief   waits on Futex Value
  //! @detail  calling thread waits on val_p_ CHECK if pointed value is testval
  //!          if testval == std::numeric_limits<int>::max() CHECK ignored
  //! @return  0 on success, -1 on error
  int Wait(int testval=std::numeric_limits<int>::max());

  //! @brief   Wakes up one/all thread(s) waiting on Futex Value
  //! @return  0 on success, -1 on error
  int Wake(bool wake_all=false);

  
  //! @brief   Returns number of threads waiting on Futex Value
  inline int Num(void) { return num_; }

  std::string to_string(void);
  friend std::ostream& operator<<(std::ostream& os, Futex& f);

 private:
  // If futex User passes Addr (ptr external integer) on which threads 
  // wait or signal:
  // Addr if passed must be 4 Byte integer aligned space. 
  // Addr is used for hashing to Futex-Q & Content of Addr is compared
  // against test_val for wait call.
  std::atomic_int* val_p_; 
  std::atomic_int  num_; // # threads waiting on val_p_
};

std::ostream& operator<<(std::ostream& os, Futex& f);
//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_FUTEX_H_
