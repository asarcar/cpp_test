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
//   int fn_orig(t1 arg1,..., th argh, ti argi,..., t(i+k) argj,...,tN argN);
// 
//   function<int(t1, ..., th, t(i+k), ..., tN)> fn_new = 
//     fn_bind(fn_orig, i, argi, ..., arg(i+k-1));
// 
//   fn_new(arg1, ..., argh, arg(i+k), ..., argN) calls 
//     fn_orig(arg1, ..., argN)
// 
// Default arguments to fn_bind: i=1
// 

// Range<1,2,...,k> = BasicRangeBindor<1, k>::type
// Range<i+1, i+2, ..., i+k>=NewRangeBindor<i,k>::type;
template <int... Args>
struct Range {
};

template<int First, int Num, int Offset, int Val, int... Args> 
struct RangeBind {
  static_assert(First > 0, "RangeBind: First > 0 failed");
  static_assert(Num > 0, "RangeBind: Num > 0 failed");
  static_assert(!(Val < 0), "RangeBind: Val >= 0 failed");
  static_assert(!(Offset < 0), "RangeBind: Offset >= 0 failed");
  using type = typename RangeBind<First, Num, Offset-1, Val-1, Val, Args...>::type;
};

template <int First, int Num, int Val, int... Args>
struct RangeBind <First, Num, 0, Val, Args...> {
  static_assert(First > 0, "RangeBind: First > 0 failed");
  static_assert(Num > 0, "RangeBind: Num > 0 failed");
  static_assert(!(Val < 0), "RangeBind: Val >= 0 failed");
  using type = Range<Args...>;
};

//
// One attempt to solve the above recursive definition could be
// to consolidate both definitions into one:
// template<int First, int Num, int Val, int... Args>
// struct RangeBind {
//   using type = 
//     Conditional<(First - Val == 1),
//                 Range<Args...>, 
//                 typename RangeBind<First, Num, Val-1, Val, Args...>::type>;
// };
// However, Conditional seems to *always* evaluate both the clauses which 
// leads to infinite recursion...
//

template<int Num>
using BasicRangeBind=typename RangeBind<1,Num,Num,Num>::type;

// Range<Base, Base+1,..., Base+Num-1> i.e. [i,i+k)
template <int Base, int Num>
using GenericRangeBind=typename RangeBind<Base, Num, Num, Base+Num-1>::type;

// Helper Class for Binding
class RangeBindor {
 public:
  // Example:
  //   SealAll(fn_locked, lck, fn_unlocked, a, b, c)
  //     fn_locked(lck, fn_unlocked(a, b, c) {
  //       lock(lck); fn_unlocked(); unlock(lck);
  //     }
  //   SealNone:
  //     fn_locked(lck, fn_unlocked) {
  //       lock(lck); fn_unlocked(); unlock(lck);
  //     }
  template <typename Ret, typename Arg, typename... Args>
  static inline std::function<Ret(void)>
  Seal(const std::function<Ret(Arg, std::function<Ret(void)>)>& fn_seal,
       const Arg& arg, const std::function<Ret(Args...)>& fn_orig,
       const Args... args) {
    // Compiler: gcc 4.8.4 croaks if bind rvalue is directly substituted below
    // without cast to std::function<Ret(void)> first
    return All(fn_seal, arg, All(fn_orig, args...));
  }
  template <typename Ret, typename... Args>
  static inline std::function<Ret(void)>
  All(const std::function<Ret(Args...)>& fn, const Args&... args) {
    return std::bind(fn, args...);
  }
  template <typename Ret, typename... Args>
  static inline std::function <Ret(Args...)>
  None(const std::function <Ret(Args...)>& fn_orig) {
    return bArgs(fn_orig, BasicRangeBind<sizeof...(Args)>());
  }
 private:
  template <typename Ret, typename... Args, int... Positions>
  static inline std::function <Ret(Args...)>
  bArgs(const std::function <Ret(Args...)>& fn_orig, 
        const Range<Positions...>& arg_pos) {
    return std::bind(fn_orig, std::_Placeholder<Positions>()...);
  }
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace misc {

#endif // _UTILS_MISC_RANGE_BINDOR_H_
