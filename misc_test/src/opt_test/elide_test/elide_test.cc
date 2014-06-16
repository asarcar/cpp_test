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

int n = 0;
struct C {
  explicit C(int) {}
  // the copy constructor has a visible side effect
  // it modifies an object with static storage duration
  C(const C&) { ++n; } 
  
  void disp(void) {std::cout << "Testing copy elide" << std::endl;}
};                     
 
void f() {
  C c(20);
  // copying the named object c into the exception object.
  // It is unclear whether this copy may be elided.
  throw c; 
}          
 
int main() {
  try {
    C c1 = C(10); // copy-initialization, calls C::C( C(42) )
    std::cout << n 
              << ((n == 0) ? ": copy elided" : ": copy not elided")
              << std::endl; //prints 0 if the copy was elided, 1 otherwise
    C c2 = c1; // explicit copy initialization
    std::cout << n 
              << ((n == 1) ? ": copy not elided" : ": copy cannot be elided")
              << std::endl; //prints 0 if the copy was elided, 1 otherwise
    c2.disp();
    f();
  }
  // copying the exception object into the temporary in the exception declaration.
  // It is also unclear whether this copy may be elided.
  catch(C c) {  
    std::cout << n 
              << ((n == 3) ? ": excep copy not elided" : ": excep copy elided")
              << std::endl; //prints 0 if the copy was elided, 1 otherwise
  }             

  return 0;
}
