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

#ifndef _UTILS_CONCUR_CONCUR_Q_H_
#define _UTILS_CONCUR_CONCUR_Q_H_

//! @file   concur_q.h
//! @brief  Concurrent Q: NonBlocking even when Pop is "tried" in empty Q
//! @detail Efficiency is realized via SpinLocks naturally assuming
//!         threads are not CPU starved - specifically SpinLock threads.
//!         This queue uses two exclusive spinlocks - one for producers
//!         other for consumers. Many concurrent queue implementations are 
//!         inspired from the following papers:
//!         1. "Simple, Fast and Practical Non-Blocking and Blocking 
//!             Concurrent Queue Algorithms", 
//!            by Maged Michael and Michael Scott, PODC 1996.
//!         2. "Obstruction-Free Synchronization: Double-Ended Queues 
//!             As an Example",
//!            by M. Herlihy, V. Luchango and M. Moir.
//!         This queue is based on Herb Sutter's work on a simple
//!         BLOCKING Q that trades performance for simplicity.
//!         http://www.drdobbs.com/parallel/
//!                measuring-parallel-performance-optimizin/212201163
//!              
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <atomic>       // std::atomic_flag
#include <iostream>     // std::ostream
#include <memory>       // std::unique_ptr
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/proc_info.h"
#include "utils/concur/lock_guard.h"
#include "utils/concur/spin_lock.h"

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename ValueType>
class ConcurQ {
 public:
  using ValueTypePtr = std::unique_ptr<ValueType>;

 private:
  struct Node {
    Node(ValueTypePtr&& val) : val_{std::move(val)}, next_(nullptr) {}
    ValueTypePtr val_;
    // atomic next_: ensures DRF (Data Race Free) as written by 
    // producers (linking new items) and read by consumers (checking 
    // whether any more items exist.
    std::atomic<Node*> next_; 
  } __attribute__ ((aligned (CACHE_LINE_SIZE)));
  
 public:
  // Constructor: assumed called from a single thread
  ConcurQ(): con_lck_{}, pro_lck_{} {
    // create sentinel node: head and tail point when Q is empty
    sentinel_ = tail_ = new Node(ValueTypePtr{});
  }
  // Destructor: assumed called from a single thread
  ~ConcurQ() {
    Node *tmpn;
    for (Node *tmp = sentinel_; tmp != nullptr; tmp = tmpn) {
      tmpn = tmp->next_;
      delete tmp->val_.release();
      delete tmp;
    }
  }
  // Prevent bad usage: copy and assignment
  ConcurQ(const ConcurQ&)             = delete;
  ConcurQ& operator =(const ConcurQ&) = delete;
  ConcurQ(ConcurQ&&)                  = delete;
  ConcurQ& operator =(ConcurQ&&)      = delete;

  // Pushes a new element to the tail of the Q.
  void Push(ValueTypePtr&& val) {
    Node* tmp = new Node(std::move(val)); // ensure node alloc succeeds, then release
    // protect critical region form all producers
    lock_guard<SpinLock> _{pro_lck_};
    tail_->next_ = tmp;
    // Never change the below below line of code to 
    //   tail_ = tail_->next
    // as tail_ may already be freed by a consumer
    // by the time we reach the following line of code
    tail_ = tmp;
    return;
  }

  // Nonblocking: 
  // TryPop: Pops the element at the head of the Q. If Q is empty returns nullptr
  ValueTypePtr TryPop(void) {
    Node* prev_sentinel;
    ValueTypePtr  val{nullptr};
    {
      // protect all consumers from critical region
      lock_guard<SpinLock> _{con_lck_};
      Node* candidate_sentinel = sentinel_->next_;
      if (candidate_sentinel == nullptr)
        return nullptr;
      prev_sentinel = sentinel_;
      sentinel_ = candidate_sentinel;
      val.swap(sentinel_->val_); // val.reset(sentinel_->val_.release())
    }
    delete prev_sentinel;
    return val; 
  }

 private:
  // Consumer Owned Data & Contention Avoidance Lock
  // Next of sentinel is head of the list
  Node*    sentinel_ __attribute__ ((aligned (CACHE_LINE_SIZE)));
  SpinLock con_lck_ __attribute__ ((aligned (CACHE_LINE_SIZE)));

  // Producer Owned Data & Contention Avoidance Lock
  Node*    tail_ __attribute__ ((aligned (CACHE_LINE_SIZE)));  
  SpinLock pro_lck_ __attribute__ ((aligned (CACHE_LINE_SIZE)));
};
//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
#endif // _UTILS_CONCUR_CONCUR_Q_H_
