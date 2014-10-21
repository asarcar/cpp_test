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

//! @file     overlap_test.cc
//! @brief    Utility to test overlap of two ranges
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <iostream>
// Standard C Headers
#include <cmath>           // abs(double)
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/math/overlap.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::math;
using namespace std;

class OverlapTester {
 public:
  OverlapTester() = default;
  ~OverlapTester() = default;
  void Run(void) {
    // Test when overlap does not exist
    r1 = std::make_pair(Type{-20}, Type{30});
    r2 = std::make_pair(Type{20}, Type{5});
    res = ComputeOverlap(r1, r2);
    CHECK_LE(res.second, 0);

    // Test when 2nd is completely overlapped by first
    r1 = std::make_pair(Type{-20}, Type{30});
    r2 = std::make_pair(Type{-10}, Type{5});
    res = ComputeOverlap(r1, r2);
    CHECK_EQ(res.first, -10);
    CHECK_EQ(res.second, 5);

    // Test when 2nd and 1st have partial overlap
    r1 = std::make_pair(Type{-20}, Type{30});
    r2 = std::make_pair(Type{5}, Type{20});
    res = ComputeOverlap(r1, r2);
    CHECK_EQ(res.first, 5);
    CHECK_EQ(res.second, 5);

    // Test Boundary Conditions: One single point overlap
    r1 = std::make_pair(Type{-20}, Type{30});
    r2 = std::make_pair(Type{-30}, Type{11});
    res = ComputeOverlap(r1, r2);
    CHECK_EQ(res.first, -20);
    CHECK_EQ(res.second, 1);

    return;
  }
 private:
  using Type=int64_t;
  using PairType = std::pair<Type,Type>;
  PairType r1;
  PairType r2;
  PairType res;
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  LOG(INFO) << argv[0] << " Executing Test";
  OverlapTester ot;
  ot.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

