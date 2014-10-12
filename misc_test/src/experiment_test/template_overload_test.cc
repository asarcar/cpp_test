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

//! @file   template_overload_test.cc
//! @brief  Template Overload tests
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <array>            // std::array
#include <iostream>         // std::cout
#include <functional>       // std::function
// C Standard Headers
// Google Headers
#include <gflags/gflags.h>  // Parse command line args and flags
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/misc/meta.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::misc;
using namespace std;

static void template_test(void);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  template_test();

  return 0;
}

template <typename T>
EnableIf<(IsReference<T>()), void> fn(const T&& t) {
  cout << "fn: args t=" << t << std::boolalpha 
       << ": reference= " << IsReference<T>()
       << ", rvalue=" << IsRValueReference<T>() << endl;
  return;
}

//! @brief: vanilla (no reference) function needed to bind to fn<int>(int)
//! @details: otherwise calls to fn<int>(rvalue or lvalue) would fail!!!
template <typename T>
EnableIf<(!IsReference<T>()), void> fn(const T t) {
  cout << "fn: args t=" << t << std::boolalpha 
       << ": reference= " << IsReference<T>() 
       << ", rvalue=" << IsRValueReference<T>() << endl;
  return;
}

template <typename F, typename T>
void call_fn(F&& f, T&& t) {
  cout << "call_fn: args f-rvalue=" << std::boolalpha 
       << IsRValueReference<F&&>() 
       << ", t-reference=" << IsReference<T&&>() 
       << ", t-rvalue=" << IsRValueReference<T&&>() << endl;
  f(forward<T>(t));
}

void template_test(void) {
  cout << "fn<int&&>(3)" << endl; 
  fn<int&&>(3);
  int i{3};
  // fn(i): inference of arg type: first tries int& and then int
  cout << "fn(i)" << endl; 
  fn(i);
  // explicit set <T> to <int&> else error: cannot bind 'int' lvalue to 'const int&&'
  // fn<int>(i): <int> specification needed
  // fn(i): inference of arg type: first tries int& and then int
  cout << "fn<int&>(i)" << endl; 
  fn<int&>(i);
  cout << "fn<int>(3)" << endl; 
  fn<int>(3);
  cout << "call_fn(function<void(int)>{fn<int>}, 3)" << endl;
  call_fn(function<void(int)>{fn<int>}, 3);
  cout << "call_fn(function<void(int&&)>{fn<int&&>}, 3)" << endl;
  call_fn(function<void(int&&)>{fn<int&&>}, 3);
  cout << "call_fn(function<void(int&)>{fn<int&>}, i)" << endl;
  call_fn(function<void(int&)>{fn<int&>}, i);
  cout << "call_fn(fn<int>, i)" << endl;
  // explicit set <T> to <int&> else error: cannot bind 'int' lvalue to 'const int&&'
  call_fn(fn<int&>, i); 
  cout << "call_fn(lambda_fn = [](int j) -> void" << endl;
  auto lambda_fn = [](int j) -> void {
    cout << "j=" << j << endl; 
    return;
  };
  call_fn(lambda_fn, i);
  cout << "call_fn([](int& j) -> void" << endl;
  call_fn([](int& j) -> void {
      cout << "j=" << j << endl; 
      return;
    }, i);
  cout << "call_fn([](int&& j) -> void" << endl;
  call_fn([](int&& j) -> void {
      cout << "j=" << j << endl; 
      return;
    }, 3);

  return;
}
