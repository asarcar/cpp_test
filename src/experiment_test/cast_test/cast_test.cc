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

namespace asarcar {
//-----------------------------------------------------------------------------
class A {
 public:
  A(int i): _i{i*2} {std::cout << "A ctor called: _i=" << _i << std::endl;} 
  void noop() {return;} // dummy fn: shut warning that last variable is used
 private:
  int _i;
};

class B {
 public:
  B(int i): _i{i} {std::cout << "B ctor called: _i=" << _i << std::endl;} 
  explicit operator A() {
    std::cout << "B to A cast called" << std::endl; 
    return A{_i};
  }
 private:
  int _i;
};

void f(A a) {
  std::cout << "Function f called " << std::endl;
}

//-----------------------------------------------------------------------------
} // namespace asarcar


using namespace asarcar;
using namespace std;

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  B b(1);
  A a = static_cast<A>(b);
  a.noop();
  f(static_cast<A>(b));

  return 0;
}

