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
#include "utils/basic/init.h"
#include "utils/misc/range_bindor.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::misc;
using namespace std;

class RangeBindorTester {
 public:
  using IntPair     = pair<int,int>;
  using ThreeIntMul = function<int(int, int, int)>;
  using ManyIntMul  = function<int(IntPair,int,int,IntPair,int)>;
  RangeBindorTester(void) : 
      mul_{
      [](IntPair v1, int a, int b, IntPair v2, int c){
        return v1.first*v1.second*a*b*v2.first*v2.second*c;
      }
  } {}
  ~RangeBindorTester(void) {}
  void Run(void) {
    LOG(INFO) << "FN_BIND: Test";
    auto mul2 = fn_bind(mul_);
    CHECK_EQ(mul2(std::make_pair(2,4),6,8,std::make_pair(10,12),14), (2*4*6*8*10*12*14));
    return;
  }
 private:
  // Bind Arguments to Positions
  template<typename Ret, typename... Args, int... Positions>
  function<Ret(Args...)> 
  fn_bind_arg_pos(function<Ret(Args...)>& fn_orig,
                  const Range<Positions...>&   arg_pos) {
    return bind(fn_orig, _Placeholder<Positions>()...);
  }

  template <typename Ret, typename... Args>
  function<Ret(Args...)>
  fn_bind(function<Ret(Args...)>& fn_orig) {
    auto range = BasicRangeBindor<sizeof...(Args)>();
    return fn_bind_arg_pos(fn_orig, range);
  }

  ManyIntMul mul_;
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  RangeBindorTester rbt;
  rbt.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
