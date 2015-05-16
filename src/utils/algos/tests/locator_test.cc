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
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/algos/locator.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::algos;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class LocatorTester {
 public:
  void TwoTest(void);
  void FullTest(void);
 private:
};

void LocatorTester::TwoTest(void) {
  array<int, 1> a1={1}, a2={2};
  DLOG(INFO) << "a1={1}, a2={2}: rank 1..3";
  CHECK_EQ(LocateRankIndex(a1, a2, 1), 0);
  CHECK_EQ(LocateRankIndex(a1, a2, 2), 1);
  CHECK(LocateRankIndex(a1, a2, 3) > 3);

  array<int, 0> a3={};
  DLOG(INFO) << "a1={1}, a3={}: rank 1..2";
  CHECK_EQ(LocateRankIndex(a1, a3, 1), 0);
  CHECK(LocateRankIndex(a1, a3, 2) > 2);

  return;
} 

void LocatorTester::FullTest(void) {
  array<int, 4> a4={2,3,4,5}, a5={1,2,3,4};
  DLOG(INFO) << "a4={2,3,4,5}, a5={1,2,3,4}: rank 1..9";
  CHECK_EQ(LocateRankIndex(a4, a5, 1), 4);
  CHECK((LocateRankIndex(a4, a5, 2) == 0) || (LocateRankIndex(a4, a5, 2) == 5));
  CHECK((LocateRankIndex(a4, a5, 3) == 0) || (LocateRankIndex(a4, a5, 3) == 5));
  CHECK((LocateRankIndex(a4, a5, 4) == 1) || (LocateRankIndex(a4, a5, 4) == 6));
  CHECK((LocateRankIndex(a4, a5, 5) == 1) || (LocateRankIndex(a4, a5, 5) == 6));
  CHECK((LocateRankIndex(a4, a5, 6) == 2) || (LocateRankIndex(a4, a5, 6) == 7));
  CHECK((LocateRankIndex(a4, a5, 7) == 2) || (LocateRankIndex(a4, a5, 7) == 7));
  CHECK_EQ(LocateRankIndex(a4, a5, 8), 3);
  CHECK(LocateRankIndex(a4, a5, 9) > 9);

  return;
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  LocatorTester lt;
  lt.TwoTest();
  lt.FullTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
