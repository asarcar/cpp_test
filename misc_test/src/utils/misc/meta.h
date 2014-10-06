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

//! @file     meta.h
//! @brief    Utilities to assist in meta programming. Taken liberally
//            from Bjarne Stroustrup. Many wrappers will be available 
//            by default from C++14

//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MISC_META_H_
#define _UTILS_MISC_META_H_

// C++ Standard Headers
#include <iostream>
#include <limits>           // std::numeric_limits
#include <functional>       // std::function, std::_Placeholder<N>
#include <type_traits>
#include <utility>          // std::pair

// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
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

//! Namespace used for all miscellaneous utility routines
namespace asarcar { namespace utils { namespace misc {
//-----------------------------------------------------------------------------

//! @brief conditional wrapper
template <bool B, typename T, typename F>
using Conditional = typename std::conditional<B,T,F>::type;

//! @brief is_fundamental wrapper
template <typename T>
constexpr bool IsFundamental(void) { return std::is_fundamental<T>::value; }

//! @brief is_pod wrapper
template <typename T>
constexpr bool IsPod(void) { return std::is_pod<T>::value; }

//! @brief is_same wrapper
template <typename T, typename U>
constexpr bool IsSame(void) { return std::is_same<T,U>::value; }

//! @brief is_integral wrapper: integer types (bool, char, int, long long, ...)
template <typename T>
constexpr bool IsIntegral(void) { return std::is_integral<T>::value; }

//! @brief is_arithmetic wrapper: integer or floating point types
template <typename T>
constexpr bool IsArithmetic(void) { return std::is_arithmetic<T>::value; }

//! @brief is_convertible wrapper
template <typename From, typename To>
constexpr bool IsConvertible(void) { return std::is_convertible<From, To>::value; }

//! @brief enable_if wrapper
template <bool B, typename T = void>
using EnableIf = typename std::enable_if<B,T>::type;

//! @brief is_reference wrapper
template <typename T>
constexpr bool IsReference(void) { return std::is_reference<T>::value; }

//! @brief is_lvalue_reference wrapper
template <typename T>
constexpr bool IsLValueReference(void) { return std::is_lvalue_reference<T>::value; }

//! @brief is_rvalue_reference wrapper
template <typename T>
constexpr bool IsRValueReference(void) { return std::is_rvalue_reference<T>::value; }

//! @brief boolean predicate combinators OR, AND, NOT
// AND
template <typename... Args>
constexpr bool All(bool b, Args... args) {
  return b && All(args...);
}

template <>
constexpr bool All(bool b) {
  return b;
}

// OR
template <typename... Args>
constexpr bool Any(bool b, Args... args) {
  return b || Any(args...);
}

template <>
constexpr bool Any(bool b) {
  return b;
}

// NOT
constexpr bool Not(bool b) {
  return !b;
}

//
// Syntactic sugar to help bind integer ranges [i,i+k) args to functions
// 
//   int fn_orig(t1 arg1, ..., th argh, ti argi, ..., t(i+k) argj, ..., tN argN);
// 
//   function<int(t1, ..., th, t(i+k), ..., tN)> fn_new = 
//     fn_bind(fn_new, i, argi, ..., arg(i+k-1));
// 
//   fn_new(arg1, ..., argh, arg(i+k), ..., argN) calls 
//     fn_orig(arg1, ..., argN)
// 
// Default arguments to fn_bind: i=1
// 

// Range<i, i+1, ..., i+k-1> = RangeFactory<i, k>::type;
template <int... Args>
struct Range {
};

template<int First, int Num, int Offset, int Val, int... Args> 
struct RangeCreator {
  static_assert(First > 0, "RangeCreator: First > 0 failed");
  static_assert(Num > 0, "RangeCreator: Num > 0 failed");
  static_assert(!(Val < 0), "RangeCreator: Val >= 0 failed");
  static_assert(!(Offset < 0), "RangeCreator: Offset >= 0 failed");
  using type = typename RangeCreator<First, Num, Offset-1, Val-1, Val, Args...>::type;
};

template <int First, int Num, int Val, int... Args>
struct RangeCreator <First, Num, 0, Val, Args...> {
  static_assert(First > 0, "RangeCreator: First > 0 failed");
  static_assert(Num > 0, "RangeCreator: Num > 0 failed");
  static_assert(!(Val < 0), "RangeCreator: Val >= 0 failed");
  using type = Range<Args...>;
};

//
// One attempt to solve the above recursive definition could be
// to consolidate both definitions into one:
// template<int First, int Num, int Val, int... Args>
// struct RangeCreator {
//   using type = 
//     Conditional<(First - Val == 1),
//                 Range<Args...>, 
//                 typename RangeCreator<First, Num, Val-1, Val, Args...>::type>;
// };
// However, Conditional seems to *always* evaluate both the clauses which 
// leads to infinite recursion...
//

template<int Num>
using BasicRangeCreator=typename RangeCreator<1,Num,Num,Num>::type;

template <int Base, int Num>
using NewRangeCreator=typename RangeCreator<Base+1, Num, Num, Base+Num>::type;

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace misc {

#endif // _UTILS_MISC_META_H_

