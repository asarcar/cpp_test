// Copyright 2016 asarcar Inc.
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
#include "utils/ds/elist.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::ds;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class ElistTester {
 public:
  void AccessTest(void) {
    CHECK_EQ(el.front(), 0);
    CHECK_EQ(el.back(), 9);
    CHECK_EQ(el.size(), 10);
  }
  void ModifyTest(void) {
    // el: {10,0,1,2,3,4,5,6,7,8,9};
    CHECK_EQ(*(el.emplace(el.begin(), 10)+1), 0);

    // el: {10,0,1,2,3,4,5,6,7,8,9,11};
    el.emplace_back(11);
    CHECK_EQ(*(el.end()-2), 9);

    // el: {10,0,1,2,12,3,4,5,6,7,8,9,11};
    CHECK_EQ(*(el.insert(el.begin()+4,12)+1),3);

    // el: {10,0,1,2,12,4,5,6,7,8,9,11};
    CHECK_EQ(*(el.erase(el.begin()+5)),4);
  }
  void IterateTest(void) {
    auto it = el.begin();
    // el: {10,0,1,2,12,4,5,6,7,8,9,11};
    CHECK_EQ(*it, 10);
    CHECK_EQ(*(it+11), 11);
    CHECK((it+12) == el.end());
  }

 private:
  Elist<int> el{0,1,2,3,4,5,6,7,8,9};
};


int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  ElistTester elt;

  elt.AccessTest();
  elt.ModifyTest();
  elt.IterateTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
