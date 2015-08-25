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

#ifndef _UTILS_CONCUR_CONCUR_BLOCK_Q_H_
#define _UTILS_CONCUR_CONCUR_BLOCK_Q_H_

//! @file   concur_block_q.h
//! @brief  Concurrent Blocking Q: Blocked at Pop (until Q is non-empty).
//! @detail Less efficient as realized via Mutex rather than SpinLocks. 
//!         However, works gracefully even when threads are contending 
//!         for CPU. Since this Queue has not been designed for very 
//!         performance hungry application, we do not worry about 
//!         cache line contention of different variables 
//!         This queue uses a mutex and condition variable
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <mutex>              // std::mutex
#include <memory>             // std::unique_ptr
#include <condition_variable> // std::condition_variable
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename ValueType>
class ConcurBlockQ {
 public:
  using ValueTypePtr = std::unique_ptr<ValueType>;
 private:
  struct Node {
    Node(ValueTypePtr&& val) : val_{std::move(val)}, next_{nullptr} {}
    ValueTypePtr  val_;
    Node*  next_;
  };
  
 public:
  ConcurBlockQ(): m_{}, cv_{} {
    // create sentinel object
    head_ = tail_ = new Node(ValueTypePtr{});
  }

  ~ConcurBlockQ() {
    Node *tmpn;
    for (Node *tmp = head_; tmp != nullptr; tmp = tmpn) {
      tmpn = tmp->next_;
      delete tmp->val_.release();
      delete tmp;
    }
  }

  void Push(ValueTypePtr&& val) {
    Node* np = new Node(std::move(val));
    {
      std::lock_guard<std::mutex> _{m_};
      tail_->next_ = np;
      tail_        = np;
    }
    // first unlock then notify to avoid spurious wakeup
    cv_.notify_one();
  }

  ValueTypePtr TryPop(void) {
    return HelperPop(true);
  }

  ValueTypePtr Pop(void) {
    return HelperPop(false);
  }

 private:
  std::mutex              m_;
  std::condition_variable cv_;
  Node*                   head_;
  Node*                   tail_;

  ValueTypePtr HelperPop(bool non_blocking) {
    Node*         prev_head;
    ValueTypePtr  val{nullptr};
    {
      // protect all consumers from critical region
      std::unique_lock<std::mutex> lk{m_};
      if (non_blocking && (head_->next_ == nullptr))
        return val;
      cv_.wait(lk, [this]{return head_->next_ != nullptr;});
      prev_head = head_;
      head_ = head_->next_;
      val.swap(head_->val_); // val.reset(sentinel_->val_.release())
    }
    delete prev_head;
    return val; 
  }
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
#endif // _UTILS_CONCUR_CONCUR_BLOCK_Q_H_
