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
#include "utils/basic/meta.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

struct NonPod {
  NonPod(int i, int j): i_{i}, j_{j} {}
  NonPod& operator+(const NonPod& other) {i_ += other.i_; j_ += other.j_; return *this;}
  int i_, j_;
};
using Pod=int;

template <typename T>
class MetaTester {
 public:
  MetaTester(T& val): v_(val) {}

  Conditional<IsPod<T>(), T, T&> getField(void) { return v_; }

  /**
   * Instantiation of template members are only triggered when there 
   * are specific calls. These calls trigger substitution and the 
   * instantiation is effected only when the substitution succeeds.
   * If the substitution fails, it is not considered an error:
   * SFINAE: Substitution Failure Is Not An Error.
   * This mechanism can be used to trigger type based declaration (note that
   * declaration triggers the success or failure of substitution: definition
   * is used in a deeper stage of template instantiation). 
   */
  template <typename U = T>
  EnableIf<(!IsPod<U>() && IsSame<T,U>()), U>* getPtr(void) { return &v_; }

  template <typename U = T>
  EnableIf<(IsPod<U>() && IsSame<T,U>()), U> getVal(void) { return v_; }

  template <typename U>
  EnableIf<(IsPod<U>() && IsPod<T>() && IsArithmetic<T>()) && IsArithmetic<U>()> 
  setVal(const U val) { v_ = val; }

 private:
  Conditional<IsPod<T>(), T, T&> v_;
};

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  Pod pod = 1;
  NonPod npod{3, 4}, npod2{5,6};
  MetaTester<Pod> mtp{pod};
  MetaTester<NonPod> mtnp{npod};

  // substitution failures 
  // auto p1 = mtp.getPtr();
  // auto v1 = mtnp.getVal();
  // mtnp.setVal(npod2);

  LOG(INFO) << "FIRST: "
            << "pod=" << pod << ": mtp.getField()=" << mtp.getField() << ": " 
            << "npod={" << npod.i_ << "," << npod.j_ << "}: " 
            << "mtnp.getField()={" 
            << mtnp.getField().i_ << "," << mtnp.getField().j_ << "}";

  CHECK_EQ(mtp.getVal(), 1);
  CHECK_EQ(mtnp.getPtr()->i_, 3);
  CHECK_EQ(mtnp.getPtr()->j_, 4);

  
  pod = 2;
  npod = npod2; // uses assignment operator
  
  LOG(INFO) << "SECOND: "
            << "pod=" << pod << ": mtp.getField()=" << mtp.getField() << ": " 
            << "npod={" << npod.i_ << "," << npod.j_ << "}: " 
            << "mtnp.getField()={" 
            << mtnp.getField().i_ << "," << mtnp.getField().j_ << "}";

  CHECK_EQ(mtp.getVal(), 1);
  mtp.setVal(2);
  CHECK_EQ(mtp.getVal(), 2);

  CHECK_EQ(mtnp.getField().i_, 5);
  CHECK_EQ(mtnp.getField().j_, 6);
  CHECK_EQ(mtnp.getPtr()->i_, 5);
  CHECK_EQ(mtnp.getPtr()->j_, 6);

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
