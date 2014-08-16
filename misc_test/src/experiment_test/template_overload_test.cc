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
// C Standard Headers
// Google Headers
#include <gflags/gflags.h>  // Parse command line args and flags
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/basic/meta.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

static void template_test(void);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  template_test();

  return 0;
}

template <typename T>
EnableIf<(IsReference<T>()), void> fn(const T&& t) {
  LOG(INFO) << "fn: args t=" << t << std::boolalpha 
            << ": reference= " << IsReference<T>()
            << ", rvalue=" << IsRValueReference<T>();
  return;
}

//! @brief: vanilla (no reference) function needed to bind to fn<int>(int)
//! @details: otherwise calls to fn<int>(lvalue) would fail!!!
template <typename T>
EnableIf<(!IsReference<T>()), void> fn(const T t) {
  LOG(INFO) << "fn: args t=" << t << std::boolalpha 
            << ": reference= " << IsReference<T>() 
            << ", rvalue=" << IsRValueReference<T>();
  return;
}

template <typename F, typename T>
void call_fn(F&& f, T&& t) {
  LOG(INFO) << "call_fn: args f-rvalue=" << std::boolalpha 
            << IsRValueReference<F&&>() 
            << ", t-reference=" << IsReference<T&&>() 
            << ", t-rvalue=" << IsRValueReference<T&&>();
  f(forward<T>(t));
}

void template_test(void) {
  LOG(INFO) << "fn<int&&>(3)"; 
  fn<int&&>(3);
  int i{3};
  LOG(INFO) << "fn<int&>(i)"; 
  // explicit set <T> to <int&> else error: cannot bind 'int' lvalue to 'const int&&'
  fn<int&>(i);
  LOG(INFO) << "fn<int>(3)"; 
  fn<int>(3);
  LOG(INFO) << "call_fn(function<void(int)>{fn<int>}, 3)";
  call_fn(function<void(int)>{fn<int>}, 3);
  LOG(INFO) << "call_fn(function<void(int&&)>{fn<int&&>}, 3)";
  call_fn(function<void(int&&)>{fn<int&&>}, 3);
  LOG(INFO) << "call_fn(function<void(int&)>{fn<int&>}, i)";
  call_fn(function<void(int&)>{fn<int&>}, i);
  LOG(INFO) << "call_fn(fn<int>, i)";
  // explicit set <T> to <int&> else error: cannot bind 'int' lvalue to 'const int&&'
  call_fn(fn<int&>, i); 
  LOG(INFO) << "call_fn(lambda_fn = [](int j) -> void";
  auto lambda_fn = [](int j) -> void {
    LOG(INFO) << "j=" << j; 
    return;
  };
  call_fn(lambda_fn, i);
  LOG(INFO) << "call_fn([](int& j) -> void";
  call_fn([](int& j) -> void {
      LOG(INFO) << "j=" << j; 
      return;
    }, i);
  LOG(INFO) << "call_fn([](int&& j) -> void";
  call_fn([](int&& j) -> void {
      LOG(INFO) << "j=" << j; 
      return;
    }, 3);

  return;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

