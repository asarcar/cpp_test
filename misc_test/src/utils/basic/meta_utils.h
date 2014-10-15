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

//! @file     meta_utils.h
//! @brief    Utilities to assist in meta programming. Taken liberally
//            from Bjarne Stroustrup. Many wrappers will be available 
//            by default from C++14

//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_META_UTILS_H_
#define _UTILS_BASIC_META_UTILS_H_

// C++ Standard Headers
#include <iostream>
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

namespace asarcar { 
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

//! @brief is_base_of wrapper
template <typename Base, typename Derived>
constexpr bool IsBaseOf(void) { return std::is_base_of<Base,Derived>::value; }

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

//! @brief is_array wrapper
template <typename T>
constexpr bool IsArray(void) { return std::is_array<T>::value; }

//! @brief extent wrapper
template <typename T>
constexpr size_t Extent(void) { return std::extent<T>::value; }

//! @brief Remove Extent available from C++14
template< class T >
using RemoveExtent = typename std::remove_extent<T>::type;

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_META_UTILS_H_

