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

//! @file     defer.h
//! @brief    Callback executor when object destroyed. 
//! @details  Implements defer() semantics natively available in go

//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MISC_DEFER_H_
#define _UTILS_MISC_DEFER_H_

// C++ Standard Headers
#include <functional>   // std::function
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/misc/range_bindor.h"

//! Namespace used for all miscellaneous utility routines
namespace asarcar { namespace utils { namespace misc {
//-----------------------------------------------------------------------------

class Defer {
 public:
  template <typename Ret, typename... Args>
  explicit Defer(std::function<Ret(Args...)> cb, Args... args) :
      cb_{RangeBindor::All(cb, args...)} {}
  ~Defer() {cb_();}
 private:
  std::function<void(void)> cb_;
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace misc {

#endif // _UTILS_MISC_DEFER_H_

