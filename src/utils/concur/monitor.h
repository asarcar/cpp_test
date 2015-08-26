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

#ifndef _UTILS_CONCUR_MONITOR_H_
#define _UTILS_CONCUR_MONITOR_H_

//! @file   monitor.h
//! @brief  Wrapper executes functions synchronously.
//! @detail Ensures calls are thread safe by taking mutual exclusion lock.
//!
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <functional>                     // std::function
#include <thread>                         // std::thread
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
class Monitor {
 private:
  mutable T              t_;
  mutable std::mutex     m_;
 public:
  explicit Monitor(T&& t): t_{std::move(t)} {}
  ~Monitor() = default;
  // Prevent bad usage: copy and assignment of Monitor
  Monitor(const Monitor&)             = delete;
  Monitor& operator =(const Monitor&) = delete;
  Monitor(Monitor&&)             = delete;
  Monitor& operator =(Monitor&&) = delete;

  template <typename F> 
  auto operator()(F f) const -> decltype(f(t_)) { 
    std::lock_guard<std::mutex> _{m_};
    return f(t_);
  }
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_MONITOR_H_
