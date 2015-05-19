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

//! @file   locator.h
//! @brief  Locates nth elements given a list of sorted arrays as arguments
//! @detail Given m sorted arrays, we are trying to find the nth elements
//!         among all those arrays.
//!         
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_LOCATOR_H_
#define _UTILS_LOCATOR_H_

// C++ Standard Headers
#include <algorithm>        // std::min
#include <array>            // std::array
#include <limits>           // std::numeric_limits
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/fassert.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace algos {
//-----------------------------------------------------------------------------

//! @fn Msb returns Ceiling(lg(r))
constexpr int 
CeilLogBase2(const size_t r) {
  return (r == 0) ? 1 : CeilLogBase2(r >> 1) + 1;
}

template <typename T, size_t N1, size_t N2>
size_t RankIndex(std::array<T, N1>& a1, std::array<T, N2>& a2, 
                 size_t x1, size_t x2) {
  if ((x1+1) == 0)
    return N1+x2;
  else if ((x2+1) == 0)
    return x1;

  // return the index i.e. max of the two elements
  if (a2.at(x2) > a1.at(x1))
    return N1+x2;

  return x1;
}

//! @fn         LocateRank: finds reference to elem with relative rank r.
//! @param[in]  Array of two sorted elements 
//! @param[in]  Rank r solicited in sorted element 
//! @param[in]  Range Search is limited to [b1,e1) and [b2, e2) range
//! @returns    Ind of the element with rank r among all elements
//!             Ind = [0, N1) => element is in array 1.
//!             Ind = [N1, N1+N2) => element is in array 2.
//!             Ind >= N1+N2 => elem with rank doesn't exist
//! Assumed: comparison operators are defined for T
template <typename T, size_t N1, size_t N2>
size_t LocateRankIndex(std::array<T, N1>& a1, 
                       std::array<T, N2>& a2, 
                       size_t r) {
  // Validate arguments
  if ((r == 0) || (r > (N1+N2)))    
    return std::numeric_limits<size_t>::max();

  // Let us binary search on the first array to find the appropriate
  // last element who is a member of the rank
  size_t e1 = std::min(a1.size(), r), e2 = std::min(a2.size(), r);
  int num_iter=0, max_iter = CeilLogBase2(r);

  // Boundary conditions: Array1 or Array2 entirely part of solution
  if (e1+e2 == r)
    return RankIndex(a1, a2, e1-1, e2-1);    
  if ((e1 == 0) || (e2 == 0) || 
      (a2.at(r-e1) >= a1.at(e1-1)))
    return RankIndex(a1, a2, e1-1, r-e1-1);
  if (a1.at(r-e2) >= a2.at(e2-1))
    return RankIndex(a1, a2, r-e2-1, e2-1);

  // b1 points to the first (min) index that is part of the range
  // assuming entire arrays are not part of the range
  size_t b1 = std::max(static_cast<size_t>(0), r - e2);
  size_t x1, x2;
  
  for (x1 = (b1+e1)/2; true; num_iter++, x1 = (b1+e1)/2) {
    // x1-th element in a1 pairs with x2-th element in a2.
    x2 = r - x1 - 2;
    DLOG(INFO) << "num_iter/max_iter=" << num_iter << "/"<< max_iter
               << ": x1/b1/e1=" << x1 << "/" << b1 << "/" << e1
               << ": x2/e2=" << x2 << "/" << e2;
    FASSERT(num_iter <= max_iter);
    FASSERT(e1 > b1);
    FASSERT(e2 > 0);

    // identify if x1 and x2 are the appropriate rank boundaries
    if (a1.at(x1) >= a2.at(x2)) {
      // elem at x1 is in between x2 and x2+1
      FASSERT(x2+1<e2); // entire subarray case already covered
      if (a1.at(x1) <= a2.at(x2+1))
        return RankIndex(a1, a2, x1, x2);
      // elements of x1 are "too high": move to next lower quanta
      e1 = x1;
    }
    // else case: (a1.at(x1) < a2.at(x2))
    else {
      // elem at x2 is in between x1 and x1+1
      FASSERT(x1+1<e1); // entire subarray case already covered
      if (a1.at(x1+1) >= a2.at(x2))
        return RankIndex(a1, a2, x1, x2);
   
      // elements of x1 are "too low": move to next higher quanta
      b1 = x1+1;
    }
  }

  // we should never reach this stage
  return std::numeric_limits<size_t>::max();
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace algos {

#endif // _UTILS_LOCATOR_H_
