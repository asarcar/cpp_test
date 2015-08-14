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

#ifndef _UTILS_CONCUR_LOCK_H_
#define _UTILS_CONCUR_LOCK_H_

//! @file   lock.h
//! @brief  Common Data Types used for Synchronization 
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <array>
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
class Lock {
 public:
  // Prevent bad usage: ctor
  Lock() = delete;

  enum class Mode : int {UNLOCK=0, SHARE_LOCK, EXCLUSIVE_LOCK};
  static inline std::string to_string(Mode mode) {
    return std::string(_toStr.at(static_cast<std::size_t>(mode)));
  }
 private:
  static constexpr 
  std::array<const char *, 
             static_cast<std::size_t>(Mode::EXCLUSIVE_LOCK)+1> _toStr 
  {{"UNLOCK", "SHARE_LOCK", "EXCLUSIVE_LOCK"}};
};

using LockMode = Lock::Mode;

inline std::ostream& operator<<(std::ostream& os, LockMode mode) {
  os << Lock::to_string(mode);
  return os;
}


//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_LOCK_H_
