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
#include "utils/misc/progeny_cast.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::misc;
using namespace std;

class ProgenyCastTester {
 public:
  ProgenyCastTester(void) {} 
  ~ProgenyCastTester(void) {}
  void Run(void) {
    LOG(INFO) << "ProgenyCast: Test"; 
    D  d;
    B* bd_p   = &d;
    B& bd_ref = d;

    D* derived_p = progeny_cast<D>(bd_p);
    CHECK_EQ(derived_p, bd_p);
    // D  *d_p   = &d;
    // D2* d2_p = progeny_cast<D2>(d_p);
    // B *base_p = progeny_cast<B>(d_p);

    D& derived_ref = progeny_cast<D>(bd_ref);
    CHECK_EQ(&derived_ref, &bd_ref);

    // D& d_ref  = d;
    // D2& d2_ref = progeny_cast<D2>(d_ref);
    // B& base_ref = progeny_cast<B>(d_ref);
    
    const D  cd;
    const B* cbd_p   = &cd;
    const B& cbd_ref = cd;

    // without progeny_cast<D> fails
    const D* cderived_p = progeny_cast<const D>(cbd_p); 
    CHECK_EQ(cderived_p, cbd_p);
    cderived_p = progeny_cast<D>(bd_p);
    CHECK_EQ(cderived_p, bd_p);

    // without progeny_cast<D> fails
    const D&  cderived_ref = progeny_cast<const D>(cbd_ref);
    CHECK_EQ(&cderived_ref, &cbd_ref);
    // cderived_ref = progeny_cast<D>(bd_ref);
    const D&  cderived_ref2 = progeny_cast<D>(bd_ref);
    CHECK_EQ(&cderived_ref2, &bd_ref);

    return;
  }
 private:
  struct B {virtual ~B(){};};
  struct D: public B {};
  struct D2: public B {};
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  ProgenyCastTester pct;
  pct.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
