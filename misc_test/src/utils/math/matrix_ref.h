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

//! @file     matrix_ref.h
//! @brief    Used to refer to any submatrix of a matrix
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MATH_MATRIX_REF_H_
#define _UTILS_MATH_MATRIX_REF_H_

// C++ Standard Headers
#include <algorithm>        // std::min
#include <iostream>
#include <sstream>          // std::stringstream
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/math/matrix_slice.h"

//! @addtogroup utils
//! @{

//! Namespace used for all math utility routines developed
namespace asarcar { namespace utils { namespace math {
//-----------------------------------------------------------------------------

// Forward Declaration
//------------------------------
template <typename T, size_t N> 
class Matrix;

template <typename T, size_t N> 
class MatrixRef;

template <typename T, size_t N>
std::ostream& operator <<(std::ostream&, const MatrixRef<T,N>&);
//------------------------------

template <typename T, size_t N>
struct MatrixRef {
  // Initalization of a ref using initializer [e.g. slice_ref_{slice}] does NOT work.
  // Instead we use braces [i.e. slice_ref_(slice)]
  explicit MatrixRef(const MatrixSlice<N>& slice, const std::vector<T>& elems):
      slice_{slice}, elems_ref_(elems){};
  std::string toString(void) const;
  friend std::ostream& operator << <>(std::ostream&, const MatrixRef<T,N>&);
  const MatrixSlice<N>  slice_;
  const std::vector<T>& elems_ref_;
};

template <typename T, size_t N>
std::string MatrixRef<T, N>::toString(void) const {
  std::ostringstream oss;
  oss << "slice_ " << slice_ << std::endl;
  oss << "elems_ref_ { "; 
  for (auto &e: elems_ref_)
    oss << e << " ";
  oss << "}" << std::endl;
  return oss.str();
}

template <typename T, size_t N>
std::ostream& operator << (std::ostream& os, const MatrixRef<T,N>& mref) {
  os << mref.toString();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace math {

#endif // _UTILS_MATH_MATRIX_REF_H_
