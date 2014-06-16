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

//! @file     fassert.h
//! @brief    Flexible Assert Class allowing one to ignore (release branch), 
//            terminate, or throw exception clauses
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _BASE_UTILS_FASSERT_H_
#define _BASE_UTILS_FASSERT_H_

// Class FAssert: Implements ideas professed by Bjarne Stroustrup
// DESCRIPTION: 
//   Extends the assert mechanism to allow flexibility where the user has the 
//   following option:
//   (1) FAssert::ReactionMode: Compile time option to determine whether assert 
//       terminates the program or throws an exception.
//   (2) FAssert::ReactionLevel: Compile time option to determine the LEVEL 
//       beyond which assertions would not be evaluated.
//   (3) MsgLevel of FAssert MsgLevel > FAssert::ReactionLevel: the assert message would
//       be evaluated. Again the ReactionLevel and the code evaluated to compare to 
//       MsgLevel is generated at compile time instead of evaluating it as runtime via 
//       generic function calls.

// C++ Standard Headers
#include <exception>       // std::terminate
#include <iostream>
#include <sstream>         // std::ostringstream
#include <stdexcept>       // std::runtime_error
// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"

//! @addtogroup utils
//! @{

//! Generic namespace used for all utility routines developed
namespace asarcar {
//-----------------------------------------------------------------------------

//! @class    FAssert
//! @brief    Flexible Assert to extend the assert mechanism to generic scenarios
class FAssert {
 public:
  // numerically higher modes more drastic modes
  enum class ReactionMode {_ignore=0, _throw=1, _terminate=2}; 
  //:-> lower the level the more its gravity
  enum class ReactionLevel {_fatal=0, _error=1, _warning=2};

#if (FASSERT_REACTION_MODE == 0)
  static constexpr ReactionMode  _cur_mode  = ReactionMode::_ignore;
#elif (FASSERT_REACTION_MODE == 1)
  static constexpr ReactionMode  _cur_mode  = ReactionMode::_throw;
#else // if (FASSERT_REACTION_MODE == 2)
  static constexpr ReactionMode  _cur_mode  = ReactionMode::_terminate;
#endif

  static constexpr ReactionLevel _def_level = ReactionLevel::_error;

#if (FASSERT_REACTION_LEVEL == 0)  
  static constexpr ReactionLevel _cur_level = ReactionLevel::_fatal;
#elif (FASSERT_REACTION_LEVEL == 1)  
  static constexpr ReactionLevel _cur_level = ReactionLevel::_error;
#else // if (FASSERT_REACTION_LEVEL == 2)
  static constexpr ReactionLevel _cur_level = ReactionLevel::_warning;
#endif

  struct Error : std::runtime_error {
    Error(const std::string &s) : std::runtime_error{s} {}
  };

  static inline constexpr bool 
  honor_level(ReactionLevel level) { 
    return (static_cast<uint32_t>(level) <= static_cast<uint32_t>(_cur_level)); 
  }

  static inline std::string 
  compose_msg(const char *file_name, uint32_t line_num, 
              const std::string &msg) {
    std::ostringstream oss;
    oss << "(" << file_name << "," << line_num << "): " << msg;
    return oss.str();
  }


  template <bool honor_assert=honor_level(_def_level), class Except = Error>
  static inline void 
  dynamic_assert(const bool& assert_condition, 
                 const std::string& msg = "FAssert::dynamic_assert failed") {
    if (assert_condition == true)
      return;
    // TODO: Replace all cerr output by dumping a LOG(msg)
    if (_cur_mode == ReactionMode::_ignore) {
      LOG(WARNING) << "fassert failure: " << msg << ": silently ignored" << std::endl;
      return;
    }
    else if (_cur_mode == ReactionMode::_terminate) {
      LOG(ERROR) << "fassert failure: " << msg << ": terminating program" << std::endl;
      std::terminate();
    }
    else { 
      LOG(ERROR) << "fassert failure: " << msg << ": throwing exception" << std::endl;
      throw Except{msg};
    }

    return;
  }
}; // class FAssert

// narrow the template definition to do NOTHING and ignore any processing 
// when honor_assert condition is not met
template <>
inline void FAssert::dynamic_assert<false, FAssert::Error>
(const bool& assert_condition, const std::string& msg) {}

#define FASSERT_LVL_MSG(lvl, exp, msg) FAssert::dynamic_assert<FAssert::honor_level(lvl), FAssert::Error>((exp), FAssert::compose_msg(__FILE__, __LINE__, "Condition failed (" #exp "): " msg))
#define FASSERT(exp) FASSERT_LVL_MSG(FAssert::ReactionLevel::_error, exp, "...")

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _BASE_UTILS_FASSERT_H_
