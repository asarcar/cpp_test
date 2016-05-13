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
using namespace std::placeholders;

class RangeBindorTester {
 public:
  using IntPair     = pair<int,int>;
  using ThreeIntMul = function<int(int, int, int)>;
  using ManyIntMul  = function<int(IntPair,int,int,IntPair,int)>;
  void SanityTest(void) {
    ManyIntMul f1 = [](IntPair v1, int a, int b, IntPair v2, int c) {
      return v1.first*v1.second*a*b*v2.first*v2.second*c;
    };
    auto f2 = RangeBindor::None(f1);
    CHECK_EQ(f2(std::make_pair(2,4),6,8,
                std::make_pair(10,12),14), (2*4*6*8*10*12*14));
    LOG(INFO) << "SanityTest Passed";
    return;
  }
  void AllArgsTest(void) {
    ManyIntMul f1 = [](IntPair v1, int a, int b, IntPair v2, int c) {
      return v1.first*v1.second*a*b*v2.first*v2.second*c;
    };
    auto f2 = RangeBindor::All(f1, std::make_pair(2,4),6,8,
                                   std::make_pair(10,12),14);
    CHECK_EQ(f2(), (2*4*6*8*10*12*14));
    LOG(INFO) << "AllArgsTest Passed";
    return;
  }
  void SealTest(void) {
    ManyIntMul f1 = [](IntPair v1, int a, int b, IntPair v2, int c) {
      return v1.first*v1.second*a*b*v2.first*v2.second*c;
    };
    function<int(int,function<int(void)>)> f2 = 
        [](int i, function<int(void)> f3) {return i*f3();};
    function<int(void)> f4 = 
        RangeBindor::Seal(f2, 16, 
                          f1, std::make_pair(2,4),6,8,
                          std::make_pair(10,12),14);
    CHECK_EQ(f4(), (2*4*6*8*10*12*14*16));
    LOG(INFO) << "SealTest Passed";
  }
 private:
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  RangeBindorTester rbt;
  rbt.SanityTest();
  rbt.AllArgsTest();
  rbt.SealTest();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
