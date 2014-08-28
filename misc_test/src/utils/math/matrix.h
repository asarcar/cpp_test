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

//! @file     matrix.h
//! @brief    Utilities to assist in matrices
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MATH_MATRIX_H_
#define _UTILS_MATH_MATRIX_H_

// C++ Standard Headers
#include <iostream>
#include <sstream>          // std::stringstream
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/math/matrix_ref.h"
#include "utils/math/matrix_priv.h"
#include "utils/math/matrix_slice.h"

//! @addtogroup utils
//! @{

//! Namespace used for all math utility routines developed
namespace asarcar { namespace utils { namespace math {
//-----------------------------------------------------------------------------


namespace um = asarcar::utils::misc;

template <typename T, size_t N>
using MatrixInitializer = typename matrix_priv::MatrixInit<T,N>::type;

// Forward Declarations
template <typename T, size_t N>
class Matrix;

template <typename T, size_t N>
std::ostream& operator<<(std::ostream&, const Matrix<T,N>&);

template <typename M>
struct MatrixType {
  constexpr static bool value = false;
};

// Specialization of MatrixType: Matrix<T,N> passed as template parameter
template <>
template <typename T, size_t N>
struct MatrixType<Matrix<T,N>> {
  constexpr static bool value = true;
};

template <typename M>
constexpr bool IsMatrixType(void) { return MatrixType<M>::value; }

// Matrix Class
template <typename T, size_t N>
class Matrix {
 public:
  using value_type = T;
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;

  template <typename U, 
            typename = um::EnableIf<um::IsConvertible<T, U>()>>
  Matrix(const MatrixRef<U,N>&); // construct from MatrixRef
  template <typename U,
            typename = um::EnableIf<um::IsConvertible<T, U>()>>
  Matrix& operator=(const MatrixRef<U,N>&); // construct from MatrixRef

  // construct Matrix with specific extents
  template <typename... Exts>
  explicit Matrix(Exts... exts) :slice_{exts...}, elems_(slice_.size()) {}

  // construct Matrix using Initializer List
  explicit Matrix(const MatrixInitializer<T, N>&);
  // prevent all other accidental use of {}
  Matrix& operator=(MatrixInitializer<T, N>) = delete;
  template <typename U>
  Matrix(std::initializer_list<U>) = delete;
  template <typename U>
  Matrix& operator=(std::initializer_list<U>) = delete;

  constexpr static size_t order(void) { return N; }
  inline size_t extent(size_t index) const { return slice_.extent(index); }
  inline size_t stride(size_t index) const { return slice_.stride(index); }
  inline size_t size(void) const { return elems_.size(); }

  // Subscript and Slice Operations
  const MatrixRef<T, N-1>& row(size_t n) const;
  const MatrixRef<T, N-1>& column(size_t n) const;
  const MatrixRef<T, N-1>& operator[](size_t index);

  template <typename... Args>
  um::EnableIf<matrix_priv::IsElementRequest<N, Args...>(), T&>
  operator()(Args... args);
      
  template <typename... Args>
  um::EnableIf<matrix_priv::IsSliceRequest<N, Args...>(), MatrixRef<T,N>>
  operator()(const Args&... args);

  std::string toString(void) const;  
  friend std::ostream& operator << <T,N>(std::ostream& os, const Matrix<T, N>& m);

  // Index Services
  inline size_t rows(size_t index) const { return slice_.rows(); }
  inline size_t cols(size_t index) const { return slice_.rows(); }
  
  // Matrix Operations
  template <typename FnObj>
  Matrix& apply(FnObj f); // apply f(x) for every element x
  template <typename M, typename FnObj>
  um::EnableIf<IsMatrixType<M>(), Matrix&> 
  apply(const M&m, FnObj f); // apply f(x, Mx) for every element x, Mx

  // Scalar Operations
  Matrix& operator=(const T& value);
  Matrix& operator+=(const T& value);
  Matrix& operator-=(const T& value);
  Matrix& operator*=(const T& value);
  Matrix& operator/=(const T& value);
  Matrix& operator%=(const T& value);

  // Matrix +, -, *
  template <typename M>
  Matrix& operator+=(const M&x);
  template <typename M>
  Matrix& operator-=(const M&x);

 private:
  MatrixSlice<N> slice_; // defines extents
  std::vector<T> elems_;
};

template <typename T>
class Matrix<T,0>; // specialization for N==0 undefined on purpose to catch errors

template <typename T, size_t N>
template <typename U, typename>
Matrix<T,N>::Matrix(const MatrixRef<U,N>& mr): 
    slice_{mr.slice_}, elems_{mr.elems_ref_}
{}

template <typename T, size_t N>
template <typename U, typename>
Matrix<T,N>& Matrix<T,N>::operator=(const MatrixRef<U,N>& mr) {
  this->slice_ = mr.slice_;
  this->elems_ = mr.elems_ref_;
  return *this;
}

template <typename T, size_t N>
Matrix<T,N>::Matrix(const MatrixInitializer<T,N>& init) {
  auto iter = slice_.extents_.begin();
  matrix_priv::DeriveExtents<N>(iter, init);
  slice_.SetDefaultStrides();
  elems_.reserve(slice_.size());
  matrix_priv::PopulateElems<N>(elems_, init);
  FASSERT(elems_.size() == slice_.size());
}

// Subscript and Slice Operations
template <typename T, size_t N>
const MatrixRef<T, N-1>& Matrix<T,N>::row(size_t n) const {
  FASSERT(n < rows());
  MatrixSlice<N-1> row;
  // matrix_priv::RefineSliceFromDim<N>(slice_, row, Slice(0), 0);
  return MatrixRef<T,N-1>{row, elems_};
}

template <typename T, size_t N>
const MatrixRef<T, N-1>& Matrix<T,N>::column(size_t n) const {
  FASSERT(n < cols());
  MatrixSlice<N-1> col;
  // matrix_priv::RefineSliceFromDim<1>(slice_, col, Slice(0), 0);
  return MatrixRef<T,N-1>{col, elems_};
}

template <typename T, size_t N>
const MatrixRef<T, N-1>& Matrix<T,N>::operator[](size_t index) { 
  return row(index); 
}

template <typename T, size_t N>
template <typename... Args>
um::EnableIf<matrix_priv::IsElementRequest<N, Args...>(), T&>
Matrix<T,N>::operator()(Args... args) {
  FASSERT(matrix_priv::AreIndicesBounded<N>(slice_.extents_, args...));
  return elems_.at(slice_(args...));
}

template <typename T, size_t N>
template <typename... Args>
um::EnableIf<matrix_priv::IsSliceRequest<N, Args...>(), MatrixRef<T,N>>
    Matrix<T,N>::operator()(const Args&... args) {
  MatrixSlice<N> new_s;
  new_s.start_ = matrix_priv::RefineSlice<N>(slice_, new_s, sizeof...(args), args...);
  MatrixRef<T,N> mr = MatrixRef<T,N>{new_s, elems_};
  DLOG(INFO) << "new_s " << new_s << ": mr " << mr;
  return mr;
}

template <typename T, size_t N>
std::string Matrix<T, N>::toString(void) const {
  std::ostringstream oss;
  oss << "slice_ " << slice_ << std::endl;
  oss << "elems_ { ";
  for (auto &elem : elems_)
    oss << elem << " ";
  oss << "}" << std::endl;
  return oss.str();
}

template <typename T, size_t N>
std::ostream& operator << (std::ostream& os, const Matrix<T, N>& m) {
  os << m.toString();
  return os;
}

template <typename T, size_t N>
template <typename FnObj>
Matrix<T,N>& Matrix<T,N>::apply(FnObj f) {
  for (auto& x: elems_) f(x); // loop should use stride iterators
  return *this; // enables chaining m.apply(abs).apply(sqrt)
}

template <typename T, size_t N>
template <typename M, typename FnObj>
um::EnableIf<IsMatrixType<M>(), Matrix<T,N>&> 
Matrix<T,N>::apply(const M &m, FnObj f) {
  // Make sure sizes and dimension match
  static_assert(order() == m.order(), "Mismatched order for this operation");
  FASSERT(AreSameExtents(slice_.extents_, m.slice_.extents_));
  for (auto iter=this->begin(), iter2=m.begin(); 
       iter != this->end();
       ++iter, ++iter2)
    f(*iter, *iter2);
  return *this;
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator=(const T& val) {
  return apply([&](T& a){ a = val; }); 
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator+=(const T& val) {
  return apply([&](T& a){ a += val; }); 
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator-=(const T& val) {
  return apply([&](T& a){ a -= val; }); 
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator*=(const T& val) {
  return apply([&](T& a){ a *= val; }); 
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator/=(const T& val) {
  FASSERT(val != 0);
  return apply([&](T& a){ a /= val; }); 
}

template <typename T, size_t N>
Matrix<T,N>& Matrix<T,N>::operator%=(const T& val) {
  return apply([&](T& a){ a %= val; }); 
}

template <typename T, size_t N>
Matrix<T,N> operator +(const Matrix<T,N>& m, const T& val) {
  Matrix<T,N> res = m;
  res += val;
  return res;
}

// Matrix Binary Operations: +, -, *
template <typename T, size_t N>
template <typename M>
Matrix<T,N>& Matrix<T,N>::operator+=(const M& m) {
  return apply(m, [](T&a, typename M::value_type &b) {a += b;});
}

template <typename T, size_t N>
template <typename M>
Matrix<T,N>& Matrix<T,N>::operator-=(const M& m) {
  return apply(m, [](T&a, typename M::value_type &b) {a -= b;});
}

#if 0
template <typename T>
Matrix<T,2> operator*(const Matrix<T,1>& u, const Matrix<T,1>& v) {
}

template <typename T>
Matrix<T,1> operator*(const Matrix<T,2>& m, const Matrix<T,1>& v) {
}

template <typename T>
Matrix<T,2> operator*(const Matrix<T,2>& m1, const Matrix<T,2>& m2) {
}
#endif

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace math {

#endif // _UTILS_MATH_MATRIX_H_
