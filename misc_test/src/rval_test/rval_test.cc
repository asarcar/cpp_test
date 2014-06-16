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
#include <iostream>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

int g1(void) {
  return 0;
}

int& g2(int& x) {
  return x;
}

int&& g3(int&& x) {
  // x has a name: subsequent references is an lvalue reference
  // hence need to convert to rvalue reference using std::move()
  return std::move(x); 
}

int main() {
  const int i0 = g1();
  int i1 = g1();
  // error rvalue cannot be bound to non-const reference: ERROR
  // int& i2 = g1();
  const int& i3 = g1();
  std::cout << "i3: " << i3 << std::endl;

  int i4 = g2(i1);
  std::cout << "i4: " << i4 << std::endl;

  int i5 = g3(20);
  std::cout << "i5: " << i5 << std::endl;

  std::cout << "g3(g1()): " << g3(g1()) << std::endl;

  int i6 = g3(g1());
  std::cout << "i6: " << i6 << std::endl;

  const int& i7 = g3(g1());
  std::cout << "i7: " << i7 << std::endl;

  const int& i8 = g2(i1);
  i1 = 5;
  std::cout << "i8: " << i8 << std::endl;

  const int& i9 = i0;
  std::cout << "i9: " << i9 << std::endl;

  return 0;
}
