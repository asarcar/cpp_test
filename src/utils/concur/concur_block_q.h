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
#include <mutex>                // std::mutex
#include <memory>               // std::unique_ptr
#include <condition_variable>   // std::condition_variable
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/meta.h"       // Conditional
#include "utils/basic/proc_info.h"  // CACHE_LINE_SIZE

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename ValueType>
class ConcurBlockQ {
 public:
  using ValueTypePtr  = std::unique_ptr<ValueType>;
  // NodeValueType: For small objects we embed the object inside
  // Any object around 1/2 the CACHE LINE SIZE is considered a small object
  // We leave 1/2 CACHE LINE SIZE for meta data and Node specific fields
  using NodeValueType = 
      Conditional<(sizeof(ValueType) <= (CACHE_LINE_SIZE >> 1)), 
                  ValueType, ValueTypePtr>;
 private:
  struct Node {
    Node(NodeValueType&& val): val_{std::move(val)}, next_{nullptr} {}
    NodeValueType   val_;
    Node*           next_;
  };
  
 public:
  ConcurBlockQ(): m_{}, cv_{} {
    // create sentinel object
    head_ = tail_ = new Node(NodeValueType{});
  }

  ~ConcurBlockQ() {
    Node *tmpn;
    for (Node *tmp = head_; tmp != nullptr; tmp = tmpn) {
      tmpn = tmp->next_;
      delete tmp; // implicit call "delete tmp->val_.release()" if unique_ptr
    }
  }

  void Push(NodeValueType&& val) {
    Node* np = new Node(std::move(val));
    {
      std::lock_guard<std::mutex> _{m_};
      tail_->next_ = np;
      tail_        = np;
    }
    // first unlock then notify to avoid spurious wakeup
    cv_.notify_one();
  }

  NodeValueType TryPop(void) {
    return HelperPop(true);
  }

  NodeValueType Pop(void) {
    return HelperPop(false);
  }

 private:
  std::mutex              m_;
  std::condition_variable cv_;
  Node*                   head_;
  Node*                   tail_;

  NodeValueType HelperPop(bool non_blocking) {
    Node*         prev_head;
    NodeValueType val{};
    {
      // protect all consumers from critical region
      std::unique_lock<std::mutex> lk{m_};
      if (non_blocking && (head_->next_ == nullptr))
        return val;
      cv_.wait(lk, [this]{return head_->next_ != nullptr;});
      prev_head = head_;
      head_ = head_->next_;
      Swap(val, head_->val_); 
    }
    delete prev_head;
    return val; 
  }

  static inline void 
  Swap(ValueTypePtr& val1, ValueTypePtr& val2) {
    val1.swap(val2); // i.e. val1.reset(val2.release())
  }
  static inline void
  Swap(ValueType& val1, ValueType& val2) {
    val1 = std::move(val2); val2 = std::move(ValueType{});
  }
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
#endif // _UTILS_CONCUR_CONCUR_BLOCK_Q_H_
