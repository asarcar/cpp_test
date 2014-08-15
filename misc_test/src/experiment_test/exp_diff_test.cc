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

//! @file   exp_test.cc
//! @brief  All experimental feature tests
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <array>            // std::array
#include <iostream>         // std::cout
// C Standard Headers
#include <cassert>          // assert
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

static void ptr_diff_test(void);
static void template_test(void);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  ptr_diff_test();
  template_test();

  return 0;
}

void ptr_diff_test(void) {
  using MyIntArray = array<int, 5>;
  using MyDoubleArray = array<double, 5>;

  MyIntArray i = {0, 1, 2, 3, 4};
  MyDoubleArray d = {0.1, 1.1, 2.1, 3.1, 4.1};

  ptrdiff_t iPtrDiff = &i[0] - &i[5];
  ptrdiff_t dPtrDiff = &d[0] - &d[5];

  ptrdiff_t ciPtrDiff = reinterpret_cast<char *>(&i[0]) - reinterpret_cast<char *>(&i[5]);
  ptrdiff_t cdPtrDiff = reinterpret_cast<char *>(&d[0]) - reinterpret_cast<char *>(&d[5]);

  MyIntArray::difference_type iDiff = i.begin() - i.end();
  MyDoubleArray::difference_type dDiff = d.begin() - d.end();

  LOG(INFO) << "  MyIntArray: array<int, 5> [0]..[4]= " << i[0] << " " << i[4]
            << ": Ptr &[0]= " << &i[0] << " &[5]= " << &i[5] 
            << ": ptrDiff &[0] - &[5]= " << iPtrDiff
            << ": byte ptrDiff &[0] - &[5]= " << ciPtrDiff
            << ": diff_type begin - end= " << iDiff
            << "  MyDoubleArray: array<double, 5> [0]..[4]= " << d[0] << " " << d[4]
            << ": Ptr &[0]= " << &d[0] << " &[5]= " << &d[5] 
            << ": ptrDiff &[0] - &[5]= " << dPtrDiff
            << ": byte ptrDiff &[0] - &[5]= " << cdPtrDiff
            << ": diff_type begin - end= " << dDiff << std::endl;

  return;
}

#define TEMPLATE_TEST 0

template <typename T>
#if (TEMPLATE_TEST == 1)
EnableIf<(IsReference<T>()), void> 
#else
void 
#endif
fn(const T&& t) {
  LOG(INFO) << "fn: args t=" << t << std::boolalpha 
            << ": reference= " << IsReference<T>()
            << ", rvalue=" << IsRValueReference<T>();
  return;
}

#if (TEMPLATE_TEST==1)
//! @brief: vanilla (no reference) function needed to bind to fn<int>(int)
//! @details: otherwise calls to fn<int>(lvalue) would fail!!!
template <typename T>
EnableIf<(!IsReference<T>()), void> fn(const T t) {
  LOG(INFO) << "fn: args t=" << t << std::boolalpha 
            << ": reference= " << IsReference<T>() 
            << ", rvalue=" << IsRValueReference<T>();
  return;
}
#endif

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

