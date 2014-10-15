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
#include <unordered_set>      // std::unordered_set
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/basic/int128_templates.h"

using namespace asarcar;
using namespace std;

class Int128TemplatesTester {
 public:
  Int128TemplatesTester(void) : 
    set{numeric_limits<int128_t>::min(), numeric_limits<int128_t>::max()} {} 
  ~Int128TemplatesTester(void) {}
  void Run(void) {
    LOG(INFO) << "INT128: Test"; 
    CHECK_EQ(set.size(), 2);
    auto it_end = set.end();
    auto it_zero = set.find(0);
    CHECK(it_zero == it_end);
    auto it_min = set.find(numeric_limits<int128_t>::min());
    CHECK(it_min != it_end);
    auto it_max = set.find(numeric_limits<int128_t>::max());
    CHECK(it_max != it_end);
    int val = (*it_min + *it_max + 1);
    // Not directly invoking CHECK_EQ(*it...) as ostream header (till GCC 4.8)
    // does not deal with 128 bit overloaded stream operators << or >>
    CHECK_EQ(val, 0);
    return;
  }
 private:
  unordered_set<int128_t> set;

  struct B {virtual ~B(){};};
  struct D: public B {};
  struct D2: public B {};
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  Int128TemplatesTester itt;
  itt.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
