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

//! @file     clock.h
//! @brief    Thin wrapper over chrono utilities

//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_CLOCK_H_
#define _UTILS_BASIC_CLOCK_H_

// C++ Standard Headers
#include <chrono>               // std::chrono::system_clock
#include <iostream>
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

namespace asarcar { 
//-----------------------------------------------------------------------------

class Clock {
 public:
  using Time=uint64_t;
  static inline Time Secs(void) {
    return std::chrono::duration_cast<std::chrono::seconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
  static inline Time USecs(void) {
    return std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
 private:
  Clock() = delete; // class is never created
};
//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_CLOCK_H_

