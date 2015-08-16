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
#include "utils/basic/clock.h"
#include "utils/basic/init.h"
#include "utils/basic/make_unique.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class MakeUniqueTester {
 public:
  void Run(void) {
    std::unique_ptr<A> p1 = make_unique<A>(2);
    CHECK_EQ(Count, 2);
    std::unique_ptr<A[]> p3 = make_unique<A[]>(3,4);
    CHECK_EQ(Count, (2+3+4));
  }
 private:
  static int Count;
  struct A {
    A(int a) : a_{a} {Count += a_;}
    int a_;
  };
};

int MakeUniqueTester::Count=0;

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  Clock::TimePoint now = Clock::USecs();
  LOG(INFO) << argv[0] << " Executing Test";
  MakeUniqueTester mut;
  mut.Run();

  LOG(INFO) << argv[0] << " Test Passed In " << (Clock::USecs() - now) << " usecs";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
