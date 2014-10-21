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

//! @file     int128_templates.h
//! @brief    Provides templates extensions for 128 bit 
//!           not available in standard unless __int128 is supported
//!           via _GLIBCXX_USE_INT128
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_INT128_TEMPLATES_H_
#define _UTILS_BASIC_INT128_TEMPLATES_H_

// C++ Standard Headers
#include <iostream>
#include <limits>           // std::numeric_limits
#include <type_traits>      // std::false_type, std::true_type, ...
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

//
// 128 bit integer is not added to standard
// Adding some trait expressions to handle 128 bits
//
namespace std {
//-----------------------------------------------------------------------------

//
// IsArithmetic which derives from IsIntegralType:
// GCC 4.8 already supports __int128 but only 
// when !defined(__STRICT_ANSI__) && defined(_GLIBCXX_USE_INT128)
//
template<>
struct __is_integral_helper<asarcar::int128_t>
    : public true_type { };

template<>
struct __is_integral_helper<asarcar::uint128_t>
    : public true_type { };

// Hash: Default function object used by many containers: e.g. ordered_set
template <>
struct hash<asarcar::int128_t> {
  size_t operator() (asarcar::int128_t value) const {
    return (value & std::numeric_limits<uint64_t>::max()) ^ (value >> 64);
  }
};  

template <>
struct hash<asarcar::uint128_t> {
  size_t operator() (asarcar::uint128_t value) const {
    return (value & std::numeric_limits<uint64_t>::max()) ^ (value >> 64);
  }
};  

// numeric_limits
template <>
struct numeric_limits<asarcar::int128_t> {
  static constexpr asarcar::int128_t min() noexcept {
    return (asarcar::int128_t(numeric_limits<int64_t>::min())) << 64;
  }
  static constexpr asarcar::int128_t max() noexcept {
    return ((asarcar::int128_t(numeric_limits<int64_t>::max())) << 64) |
        numeric_limits<int64_t>::max();
  }
};

template <>
struct numeric_limits<asarcar::uint128_t> {
  static constexpr asarcar::uint128_t min() noexcept {
    return 0;
  }
  static constexpr asarcar::uint128_t max() noexcept {
    return ((asarcar::uint128_t(numeric_limits<uint64_t>::max())) << 64) |
        numeric_limits<uint64_t>::max();
  }

};

//-----------------------------------------------------------------------------
} // namespace std

#endif // _UTILS_BASIC_INT128_TEMPLATES_H_
