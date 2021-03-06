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

#ifndef _UTILS_CONCUR_CONCUR_H_
#define _UTILS_CONCUR_CONCUR_H_

//! @file   concur.h
//! @brief  Wrapper executes functions asynchronously on another thread.
//! @detail Ensures calls are thread safe and non-blocking on the calling
//!         thread. It serializing the execution by running blocking calls
//!         on another thread that is owned by the wrapper class. 
//!         Herb Sutter: Concurrency.
//!
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <functional>           // std::function
#include <future>               // std::future
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/concur/concur_block_q.h"  // std::lock_guard

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename T>
class Concur {
  using Fn  = std::function<void()>;
  using Cbq = ConcurBlockQ<Fn>;
 private:
  mutable T     t_; // decltype reference needs t_ defined first
  mutable Cbq   q_;
  // except one time initialization, done_ is only examined or written
  // in the context of the helper thread
  bool          done_;
  // all functions enQd to q_ are executed helper thd_'s context
  // helper_thd_ keeps deQing functions until done_ is set to true 
  std::thread   helper_thd_; 
 public:
  explicit Concur(T&& t): 
    t_{std::move(t)}, q_{}, done_{false},
    helper_thd_{std::thread([=](){while(!done_) q_.Pop()();})}
  {}
  // enQ a function that terminates helper thread loop by setting done_
  // (done_ is set in the context of helper_thd_. Hence there is 
  // no race condition). wait for the helper_thd_ to terminate execution
  ~Concur() {
    q_.Push([=](){done_ = true;});
    helper_thd_.join();
  }
  // Prevent bad usage: copy and assignment of Monitor
  Concur(const Concur&)             = delete;
  Concur& operator =(const Concur&) = delete;
  Concur(Concur&&)                  = delete;
  Concur& operator =(Concur&&)      = delete;

  template <typename F> 
  auto operator()(F f) const -> std::future<decltype(f(t_))> { 
    using ReturnType = decltype(f(t_));
    using PromiseType = std::promise<ReturnType>;
    using FutureType  = std::future<ReturnType>;
    using ShPtrPrType = std::shared_ptr<PromiseType>;
    ShPtrPrType pro_p = std::make_shared<PromiseType>();
    FutureType  fut   = pro_p->get_future();
    // Until capture rvalue is not implemented we have to have some way to 
    // ensure the promise is not destroyed before the helper thread
    // can set the promise
    q_.Push([=]() {
        try { SetValue(f, *pro_p); }
        catch (...) { pro_p->set_exception(std::current_exception()); }
      });
    return fut;
  }
 private:
  template <typename F, typename Ret>
  inline void SetValue(F& f, std::promise<Ret> &pro) const {
    pro.set_value(f(t_));
  }
  template <typename F>
  inline void SetValue(F& f, std::promise<void> &pro) const {
    f(t_);
    pro.set_value();
  }
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_CONCUR_H_
