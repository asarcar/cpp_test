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

//! @file     udl.h
//! @brief    User Defined Literals for common types
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_MISC_UDU_H_
#define _UTILS_MISC_UDU_H_

// C++ Standard Headers
#include <iostream>
#include <string>           // string, to_string, ...
// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace misc {
//-----------------------------------------------------------------------------

// Credit: Concept, Implementation, etc. Verbatim: Bjarne Stroustrup

// SI Units: 3 dimensions {distance, weight, time}

// C++11 is not allowing direct template declaration of enum
// template <int M, int K, int S> enum Unit... results in error
template <int M, int K, int S>
struct Unit {
  enum Dim {m=M, kg=K, s=S};
  /**
   * Kludge: We should declare a template struct is_unit that
   * defines value as false and then specialize is_unit that 
   * is true when Unit is passed. 
   * Have not yet figured out how to do so on Unit that is 
   * itself templatized.
   */
  constexpr static bool value = true; 
};

template <typename U> 
constexpr bool IsUnitType(void) {
  return U::value;
}

//! @brief Unit created by multiplying two Quantities
template <typename U1, typename U2>
using UnitPlus=Unit<(U1::Dim::m+U2::Dim::m), 
                    (U1::Dim::kg+U2::Dim::kg), 
                    (U1::Dim::s+U2::Dim::s)>;

//! @brief Unit created by dividing two Quantities
template <typename U1, typename U2>
using UnitMinus =Unit<(U1::Dim::m-U2::Dim::m), (U1::Dim::kg-U2::Dim::kg), (U1::Dim::s-U2::Dim::s)>;

//! @brief Unit created by raising Quantities to power N
template <typename U, int N>
using UnitPower =Unit<(U::Dim::m*N), (U::Dim::kg*N), (U::Dim::s*N)>;

using M   = Unit<1, 0, 0>;     // Meters:          Distance
using K   = Unit<0, 1, 0>;     // Kilograms:       Mass
using S   = Unit<0, 0, 1>;     // Seconds:         Time
using MpS = UnitMinus<M, S>;   // Distance/Time    Speed           
using Acc = UnitMinus<MpS, S>; // Speed/Time       Acceleration
using F   = UnitPlus<K, Acc>;  // Mass*Acc         Force
using E   = UnitPlus<F, M>;    // Force*Distance   Energy

using ScalarType = double; // SI Scalar Type


// Quantity: Dimensioned Measure <Unit, Value>
template <typename U>
struct Quantity {
  static_assert(IsUnitType<U>(), "Invalid Template Parameter (not a Unit) passed to Quantity");
  constexpr explicit Quantity(ScalarType val): v{val}{}
  ScalarType v;
};

// SI User Defined Literals
constexpr Quantity<M> operator"" _m(long double val) {return Quantity<M>(val);}
constexpr Quantity<K> operator"" _kg(long double val) {return Quantity<K>(val);}
constexpr Quantity<S> operator"" _s(long double val) {return Quantity<S>(val);}

// Unit Display Helper Routine

// Unit Operators +,-,*,/,power
template <typename U>
Quantity<U> operator+(const Quantity<U>& v1, const Quantity<U>& v2) {
  return Quantity<U>{v1.v + v2.v};
}

template <typename U>
Quantity<U> operator-(const Quantity<U>& v1, const Quantity<U>& v2) {
  return Quantity<U>{v1.v - v2.v};
}

template <typename U1, typename U2>
Quantity<UnitPlus<U1,U2>> operator*(const Quantity<U1>& v1, const Quantity<U2>& v2) {
  return Quantity<UnitPlus<U1, U2>>{v1.v*v2.v};
}

template <typename U1, typename U2>
Quantity<UnitMinus<U1,U2>> operator/(const Quantity<U1>& v1, const Quantity<U2>& v2) {
  return Quantity<UnitMinus<U1, U2>>{v1.v/v2.v};
}

template <int N, typename U>
Quantity<UnitPower<U, N>> power(const Quantity<U>& v1) {
  return Quantity<UnitPower<U, N>>{v1.v^N};
}

template <typename U>
bool operator ==(const Quantity<U>& v1, const Quantity<U> &v2) {
  return v1.v == v2.v;
}

inline std::string fmt_dim(int dim, const char* fmt) {
  return ((dim == 0) ? 
          std::string("") :
          ((dim == 1) ?
           std::string(fmt) :
           std::string(fmt) + std::to_string(dim)));
}

template <typename U>
std::ostream& operator<<(std::ostream& os, const Quantity<U> &q) {
  os << q.v
     << fmt_dim(U::Dim::m,  ".m") 
     << fmt_dim(U::Dim::kg, ".kg")
     << fmt_dim(U::Dim::s,  ".s");
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace misc {


#endif // _UTILS_MISC_UDU_H_
