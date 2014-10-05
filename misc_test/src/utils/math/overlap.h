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

//! @file     overlap.h
//! @brief    Utility to assist in overlap computation of two ranges
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MATH_OVERLAP_H_
#define _UTILS_MATH_OVERLAP_H_

// C++ Standard Headers
#include <iostream>
#include <utility>      // std::pair
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/misc/meta.h"

//! Namespace used for all math utility routines developed
namespace asarcar { namespace utils { namespace math {
//-----------------------------------------------------------------------------

namespace um = asarcar::utils::misc;

//! @brief computes the overlap of two ranges: 
//         range [base, length) is denoted by pair <first, length>
//         length of overlap is <=0 if overlap does not exist
//! @param  [in] range1 std::pair<from_index, length>
//! @param  [in] range2 std::pair<from_index, length>
//! @return range std::pair<from_index, length>
template <typename T>
um::EnableIf<(um::IsIntegral<T>() && !um::IsSame<T, bool>()), std::pair<T,T>> 
ComputeOverlap(const std::pair<T,T>& p1, const std::pair<T,T>& p2) {
  FASSERT(p1.second>0); FASSERT(p2.second > 0);
  auto first = std::max(p1.first, p2.first);
  return std::make_pair(first, 
                        (std::min(p1.first + p1.second, p2.first + p2.second) - first));
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace math {

#endif // _UTILS_MATH_OVERLAP_H_
