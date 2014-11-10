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

//! @file     matrix_slice.h
//! @brief    Liberally borrowed from generalized slice. Used to index sub-arrays
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MATH_MATRIX_SLICE_H_
#define _UTILS_MATH_MATRIX_SLICE_H_

// C++ Standard Headers
#include <array>            // array
#include <initializer_list> // initializer_list
#include <iostream>
#include <limits>           // std::numeric_limits
#include <numeric>          // inner_product
#include <sstream>          // std::stringstream
// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/meta.h"

//! @addtogroup utils
//! @{

//! Namespace used for all math utility routines developed
namespace asarcar { namespace utils { namespace math {
//-----------------------------------------------------------------------------

// Forward Declarations
template <int>
class MatrixSlice;

template<int N>
std::ostream& operator<<(std::ostream& os, const MatrixSlice<N>& ms);

//----------------------------------------------------------------

struct Slice {
 public:
  explicit Slice(size_t start=std::numeric_limits<size_t>::max(), 
                 size_t length=std::numeric_limits<size_t>::max(), 
                 size_t stride=1) :
      start_{start}, length_{length}, stride_{stride} {}
  size_t start_;
  size_t length_;
  size_t stride_;
};

std::ostream& operator<<(std::ostream& os, const Slice& s) {
  os << "start_ " << s.start_ << ": length_ " << s.length_
     << ": stride_ " << s.stride_ << std::endl;
  return os;
}

template <int N>
struct MatrixSlice {
 public:
  using iterator = typename std::array<size_t, N>::iterator;
  using const_iterator = typename std::array<size_t, N>::const_iterator;

  MatrixSlice() = default; // used when empty matrix is created

  explicit MatrixSlice(size_t start, 
                       const std::initializer_list<size_t>& extents, 
                       const std::initializer_list<size_t>& strides);

  template <typename... Exts, 
            typename = EnableIf<((sizeof...(Exts) == N) && 
                                 All(IsConvertible<Exts, size_t>()...))>>
      explicit MatrixSlice(Exts... exts);

  inline size_t size(void) { 
    return std::accumulate(extents_.begin(), extents_.end(), 1, 
                           [](size_t x, size_t y){return x*y;});
  }
  
  template <typename... Dims,
            typename = EnableIf<All(IsConvertible<Dims, size_t>()...)>>
  size_t operator()(Dims... dims) const;
  
  inline size_t extent(size_t index) const { return extents_.at(index); }
  inline size_t stride(size_t index) const { return strides_.at(index); }

  inline size_t rows(size_t index) const { return extent(0); }
  inline size_t cols(size_t index) const { return extent(1); }

  std::string toString(void) const;
  friend std::ostream& operator<< <>(std::ostream& os, const MatrixSlice<N>& ms);
  void SetDefaultStrides(void); 

  size_t start_;
  std::array<size_t, N> extents_;
  std::array<size_t, N> strides_;
};

template <int N>
MatrixSlice<N>::MatrixSlice(size_t start, 
                            const std::initializer_list<size_t>& extents, 
                            const std::initializer_list<size_t>& strides) :
  start_{start} {
  FASSERT((extents.size() == strides.size()) && (extents.size() == N));
  std::copy(extents.begin(), extents.end(), extents_.begin());
  std::copy(strides.begin(), strides.end(), strides_.begin());
} 

template <int N>
template <typename... Exts, typename>
MatrixSlice<N>::MatrixSlice(Exts... exts) :
  start_{0}, 
  extents_{size_t(exts)...} {
  static_assert((sizeof...(Exts) == N), "");
  SetDefaultStrides();
} 

template <int N> 
template <typename... Dims, typename>
size_t MatrixSlice<N>::operator()(Dims... dims) const {
  static_assert((sizeof...(Dims) == N), "");
  std::array<size_t, N> args{size_t(dims)...};
  return std::inner_product(args.begin(), args.end(), 
                            strides_.begin(), 0);
}

template <int N>
std::string MatrixSlice<N>::toString(void) const {
  std::ostringstream oss;
  oss << "start_ " << start_ << ": extents_ { ";
  for (auto &ext : extents_)
    oss << ext << " ";
  oss << "} : strides_ { ";
  for (auto &str : strides_)
    oss << str << " ";
  oss << "}";
  return oss.str();
}

template <int N>
void MatrixSlice<N>::SetDefaultStrides(void) {
  size_t str = 1;
  for (int idx=N-1; idx>=0; --idx) {
    strides_[idx] = str;
    str *= extents_.at(idx);
  }
  return;
}


template<int N>
std::ostream& operator<<(std::ostream& os, const MatrixSlice<N>& ms) {
  os << ms.toString();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace math {

#endif // _UTILS_MATH_MATRIX_SLICE_H_
