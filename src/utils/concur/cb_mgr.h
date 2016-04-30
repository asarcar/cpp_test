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

//! @file   cb_mgr.h
//! @brief  Manages callback so that post free of objests it is not executed
//! @detail Example Usage:
//!         class Example {
//!           private:
//!             cb_mgr_;
//!           public:
//!           ~Example() {
//!             // Pending cbs: Example destruction proceeds after cb runs.
//!             // New cbs: Disable execution after class Example destruction.
//!             CB_QUASH_N_WAIT(cb_mgr_); 
//!           }
//!           // unsafe to call NakedCb routines after class Example is destroyed
//!           void NakedCb() {...} 
//!           // safe to call SafeFn even when CbExample
//!           void Method() {
//!             CbMgr::Closure cb = cb_mgr_.Seal(bind(&Example::NakedCb, this));
//!             // Safe to pass cb to ThreadMgr or Scheduler or any external module
//!           }
//!           
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_CONCUR_CB_MGR_H_
#define _UTILS_CONCUR_CB_MGR_H_

// C++ Standard Headers
#include <functional>   // std::function
#include <memory>       // std::make_shared
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup concur
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

template <typename F = std::function<void(void)>>
class CbMgr {
 public:
  CbMgr();
  ~CbMgr()                         = default;
  CbMgr(const CbMgr& o)            = delete;
  CbMgr& operator=(const CbMgr& o) = delete;
  CbMgr(CbMgr&& o)                 = delete; 
  CbMgr& operator=(CbMgr&& o)      = delete;
  
  // Closure passed is returned as sealed to guarantee execution "safely."
  // i.e. cb execution is linked to the lifetime of the parent object
  // which contains the cb_mgr_
  inline void Seal(F* naked_cb_p) {
    *naked_cb_p = std::bind(SealedCb, cb_p_, std::move(*naked_cb_p));
  }
  // Quash future callbacks & Wait for all outstanding callbacks to complete
  void QuashNWait(const char* file_name, int line_num);
 private:
  struct CbState; 
  using CbStatePtr = std::shared_ptr<CbState>;
  // cb_state_p is passed as first/base argument to all "sealed" callbacks.
  // Post CbMgr is destroyed we ensure that callbacks are noops.
  CbStatePtr cb_p_;

  // Arguments to bind are copied/moved: never passed by reference 
  // unless wrapped in std::[c]ref. As such, SealedCb may accept
  // smart pointer argument as reference to shared_ptr to avoid
  // yet another reference counting
  static void SealedCb(CbStatePtr&, const F&);
};

#define CB_QUASH_N_WAIT(cb_mgr) (cb_mgr).QuashNWait(__FILE__, __LINE__)

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_CB_MGR_H_

