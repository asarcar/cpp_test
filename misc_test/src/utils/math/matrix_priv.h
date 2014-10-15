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

//! @file     matrix_priv.h
//! @brief    Used for all private implementation related details
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MATH_MATRIX_PRIV_H_
#define _UTILS_MATH_MATRIX_PRIV_H_

// C++ Standard Headers
#include <array>            // array
#include <initializer_list> // initializer_list
#include <iostream>
#include <limits>           // std::numeric_limits
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"

//! @addtogroup utils
//! @{

//! Namespace used for private matrix implementation details
namespace asarcar { namespace utils { namespace math { namespace matrix_priv {
//-----------------------------------------------------------------------------

template <typename T, size_t N>
struct MatrixInit {
  using type = typename std::initializer_list<typename MatrixInit<T,N-1>::type>;
  using const_iterator = typename type::const_iterator;
};

template <typename T>
struct MatrixInit<T,1> {
  using type = typename std::initializer_list<T>;
};

template <typename T>
struct MatrixInit<T,0>; // declared but undefined to catch errors

/**
 * function template partial specialization: not allowed.
 * Otherwise: DeriveExtents and PopulateElems could be written using 
 * specialization logic with any N and then N equal to 1
 */
template <size_t N, typename I, typename List>
EnableIf<(N==1)> DeriveExtents(I& iter, const List& list) {
  *iter = list.size();
  return;
}

template <size_t N, typename I, typename List>
EnableIf<(N>1)> DeriveExtents(I& iter, const List& list) {
  *iter = list.size();
  DeriveExtents<(N-1)>(++iter, *list.begin());
  return;
}

template <size_t N, typename T, typename List>
EnableIf<(N==1)> 
PopulateElems(std::vector<T>& vec, const List& list) {
  vec.insert(vec.end(), list.begin(), list.end());
  return;
}

template <size_t N, typename T, typename List>
EnableIf<(N>1)> 
PopulateElems(std::vector<T>& vec, const List& list) {
  for(auto iter=list.begin(); iter != list.end(); ++iter)
    PopulateElems<N-1>(vec, *iter);
  return;
}

template <size_t N, typename... Args>
constexpr bool IsElementRequest() {
  return (All(IsConvertible<Args, size_t>()...) && (sizeof...(Args) == N));
}

template <size_t N, typename... Args>
constexpr bool IsSliceRequest() {
  return (All((IsConvertible<Args, size_t>() || 
               IsSame<Args, Slice>())...) &&
          // At least one Arg should be of type Slice
          (Any(IsSame<Args, Slice>()...)) && 
          (sizeof...(Args) <= N));
}

template <size_t N, typename... Args>
bool AreIndicesBounded(std::array<size_t, N>& extents_, Args... args) {
  std::array<size_t, N> indices {size_t(args)...};
  // all the indices should be less than what is specified in extents
  return std::equal(indices.begin(), indices.end(), extents_.begin(), std::less<size_t>{});
}

template <size_t N>
bool AreSameExtents(const std::array<size_t, N>& s1, 
                    const std::array<size_t, N>& s2) {
  return std::equal(s1.begin(), s1.end(), s2.begin(), std::equal_to<size_t>{});
}

//! @brief Compute the extents and strides for the new slice for dimension
//         number Dim. Return the contribution for start offset
template <size_t N>
size_t RefineSliceForDim(const MatrixSlice<N>& orig_s, 
                         MatrixSlice<N>& new_s,
                         const Slice& s,
                         const size_t dim) {
  // TODO: Need to fix code to compute overlap of slices
  new_s.extents_.at(dim) = std::min<size_t>(s.length_, orig_s.extents_.at(dim));
  new_s.strides_.at(dim) = std::max<size_t>(s.stride_, orig_s.strides_.at(dim));
  size_t dim_num = std::max<size_t>((orig_s.start_ / orig_s.strides_.at(dim)), s.start_);
  size_t start_offset = dim_num * orig_s.strides_.at(dim);
  DLOG(INFO) << "orig_s " << orig_s << ": new_s " << new_s 
             << ": slice " << s << ": dim " << dim 
             << ": start_offset " << start_offset;
  return start_offset;
}

template <size_t N>
size_t RefineSlice(const MatrixSlice<N>& orig_s, MatrixSlice<N>& new_s, size_t num_args) {
  return 0;
}

template <size_t N, typename T, typename... Args>
size_t RefineSlice(const MatrixSlice<N>& orig_s, MatrixSlice<N>& new_s, 
                   const size_t num_args,
                   const T& s, const Args&... args) {
  size_t m = RefineSliceForDim<N>(orig_s, new_s, Slice(s), (num_args-sizeof...(Args)-1));
  size_t n = RefineSlice<N>(orig_s, new_s, num_args, args...);
  return m+n;
}

//-----------------------------------------------------------------------------
} } } } // namespace asarcar { namespace utils { namespace math { matrix_priv

#endif // _UTILS_MATH_MATRIX_PRIV_H_

