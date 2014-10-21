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

//! @file     range_bindor.h
//! @brief    Utilities to assist in binding a range of params. 
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MISC_RANGE_BINDOR_H_
#define _UTILS_MISC_RANGE_BINDOR_H_

// C++ Standard Headers
#include <iostream>
#include <functional>       // std::function, std::_Placeholder<N>

// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

//! Namespace used for all miscellaneous utility routines
namespace asarcar { namespace utils { namespace misc {
//-----------------------------------------------------------------------------
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
struct RangeBindor {
  static_assert(First > 0, "RangeBindor: First > 0 failed");
  static_assert(Num > 0, "RangeBindor: Num > 0 failed");
  static_assert(!(Val < 0), "RangeBindor: Val >= 0 failed");
  static_assert(!(Offset < 0), "RangeBindor: Offset >= 0 failed");
  using type = typename RangeBindor<First, Num, Offset-1, Val-1, Val, Args...>::type;
};

template <int First, int Num, int Val, int... Args>
struct RangeBindor <First, Num, 0, Val, Args...> {
  static_assert(First > 0, "RangeBindor: First > 0 failed");
  static_assert(Num > 0, "RangeBindor: Num > 0 failed");
  static_assert(!(Val < 0), "RangeBindor: Val >= 0 failed");
  using type = Range<Args...>;
};

//
// One attempt to solve the above recursive definition could be
// to consolidate both definitions into one:
// template<int First, int Num, int Val, int... Args>
// struct RangeBindor {
//   using type = 
//     Conditional<(First - Val == 1),
//                 Range<Args...>, 
//                 typename RangeBindor<First, Num, Val-1, Val, Args...>::type>;
// };
// However, Conditional seems to *always* evaluate both the clauses which 
// leads to infinite recursion...
//

template<int Num>
using BasicRangeBindor=typename RangeBindor<1,Num,Num,Num>::type;

template <int Base, int Num>
using NewRangeBindor=typename RangeBindor<Base+1, Num, Num, Base+Num>::type;

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace misc {

#endif // _UTILS_MISC_RANGE_BINDOR_H_
