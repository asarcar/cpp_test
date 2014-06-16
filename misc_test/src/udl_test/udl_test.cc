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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <complex>
#include <iostream>
#include <string>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

namespace asarcar {
//-----------------------------------------------------------------------------
std::string operator"" _s(const char *s, size_t len) {
  return std::string(s);
}

using DimTypeD = long double;
using ComplexD = std::complex<DimTypeD>;

constexpr ComplexD operator"" _r(DimTypeD realval) {
  static_assert(std::is_pod<DimTypeD>::value, "DimTypeD should be a POD");
  return ComplexD{realval, 0};
}

constexpr ComplexD operator"" _i(DimTypeD imgval) {
  static_assert(std::is_pod<DimTypeD>::value, "DimTypeD should be a POD");
  return ComplexD{0, imgval};
}

using DimTypeZ = unsigned long long;
using ComplexZ = std::complex<DimTypeZ>;

constexpr ComplexZ operator"" _r(DimTypeZ realval) {
  static_assert(std::is_pod<DimTypeZ>::value, "DimTypeZ should be a POD");
  return ComplexZ{realval, 0};
}

constexpr ComplexZ operator"" _i(DimTypeZ imgval) {
  static_assert(std::is_pod<DimTypeZ>::value, "DimTypeZ should be a POD");
  return ComplexZ{0, imgval};
}

//-----------------------------------------------------------------------------
} // namespace asarcar


using namespace asarcar;
using namespace std;

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);
  
  std::string str = "Hello"_s + " World!"_s;
  std::cout << str << std::endl;

  ComplexD c = 3.0_r + 4.0_i;
  std::cout << "3.0_r + 4.0_i = " << c << std::endl;

  ComplexZ d = 6_r + 8_i;
  std::cout << "6_r + 8_i = " << d << std::endl;

  return 0;
}

