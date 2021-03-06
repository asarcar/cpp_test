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
#include <limits>       // std::numeric_limits<>::max()
#include <chrono>       // std::chrono::system_clock
#include <iostream>
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

namespace asarcar { 
//-----------------------------------------------------------------------------

class Clock {
 public:
  using TimePoint    = uint64_t;
  using TimeDuration = TimePoint;
  using TimeUSecs    = std::chrono::microseconds;
  using TimeMSecs    = std::chrono::milliseconds;
  using TimeSecs     = std::chrono::seconds;
  Clock() = delete; // class is never created
  static inline TimePoint USecs(void) {
    return std::chrono::duration_cast<TimeUSecs>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
  static inline TimePoint MSecs(void) {
    return std::chrono::duration_cast<TimeMSecs>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
  static inline TimePoint Secs(void) {
    return std::chrono::duration_cast<TimeSecs>
        (std::chrono::system_clock::now().time_since_epoch()).count();
  }
  static inline TimeDuration MaxDuration() {
    return std::numeric_limits<TimeDuration>::max();
  }
 private:
};
//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_CLOCK_H_

