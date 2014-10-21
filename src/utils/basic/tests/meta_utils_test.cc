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
#include <array>
#include <functional>         // std::function
#include <initializer_list>
#include <iostream>
#include <unordered_set>      // std::unordered_set
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/basic/meta_utils.h"

using namespace asarcar;
using namespace std;

template <typename T>
class MetaClass {
 public:
  MetaClass(T& val): v_(val) {}

  Conditional<IsFundamental<T>(), T, T&> getField(void) { return v_; }

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
  EnableIf<(!IsFundamental<U>() && IsSame<T,U>()), U>* getPtr(void) { return &v_; }

  template <typename U = T>
  EnableIf<(IsFundamental<U>() && IsSame<T,U>()), U> getVal(void) { return v_; }

  template <typename U>
  EnableIf<(IsFundamental<U>() && IsFundamental<T>() && IsArithmetic<T>()) && IsArithmetic<U>()> 
  setVal(const U val) { v_ = val; }

 private:
  Conditional<IsFundamental<T>(), T, T&> v_;
};

class MetaTester {
 public:
  MetaTester(void) : 
    fundamental_{1}, nfundamental_{1,2,3,4},
    mtf_{fundamental_}, mtnf_{nfundamental_} {}
  ~MetaTester(void) {}
  void Run(void) {
    LOG(INFO) << "NonFundamental: " << boolalpha << IsFundamental<NonFundamental>()
              << " Fundamental: " << IsFundamental<Fundamental>() 
              << " All(IsPod<int>, IsArithmetic<size_t>, IsFundamental<float>): "
              << All(IsPod<int>(), IsArithmetic<size_t>(), IsFundamental<float>())
              << " All(IsPod<int>, IsArithmetic<size_t>, IsFundamental<NonFundamental>): "
              << All(IsPod<int>(), IsArithmetic<size_t>(), IsFundamental<NonFundamental>())
              << " Any(IsPod<int>, IsArithmetic<size_t>, IsFundamental<NonFundamental>): "
              << Any(IsPod<int>(), IsArithmetic<size_t>(), IsFundamental<NonFundamental>())
              << " Not(IsFundamental<NonFundamental>()): "
              << Not(IsFundamental<NonFundamental>());
    LOG(INFO) << "Convertible<int, float>: " << boolalpha << IsConvertible<int, float>()
              << " Convertible<double, int>: " << IsConvertible<double, int>()
              << " Convertible<int, NonFundamental>: " 
              << IsConvertible<int, NonFundamental>();
    
    // Substitution Failures
    // auto p1 = mtf.getPtr();
    // auto v1 = mtnf.getVal();

    LOG(INFO) << "FIRST: "
              << "fundamental=" << fundamental_ << ": mtf.getField()=" << mtf_.getField() << ": " 
              << "nfundamental={" << nfundamental_.v_[0] << "," << nfundamental_.v_[1]
              << "," << nfundamental_.v_[2] << "," << nfundamental_.v_[3] << "}: " 
              << "mtnf.getField()={" 
              << mtnf_.getField().v_[0] << "," << mtnf_.getField().v_[1]
              << "," << mtnf_.getField().v_[2] << "," << mtnf_.getField().v_[3] << "}";
    
    CHECK_EQ(mtf_.getVal(), 1);
    CHECK_EQ(mtnf_.getPtr()->v_[0], 1);
    CHECK_EQ(mtnf_.getPtr()->v_[1], 2);
    
  
    fundamental_ = 2;
    nfundamental_ = {5,6,7,8}; // uses assignment operator
    
    LOG(INFO) << "SECOND: "
              << "fundamental=" << fundamental_ << ": mtf.getField()=" << mtf_.getField() << ": " 
              << "nfundamental={" << nfundamental_.v_[0] << "," << nfundamental_.v_[1]
              << "," << nfundamental_.v_[2] << "," << nfundamental_.v_[3] << "}: " 
              << "mtnf.getField()={" 
              << mtnf_.getField().v_[0] << "," << mtnf_.getField().v_[1]
              << "" << mtnf_.getField().v_[2] << "," << mtnf_.getField().v_[3] << "}";

    CHECK_EQ(mtf_.getVal(), 1);
    mtf_.setVal(2);
    CHECK_EQ(mtf_.getVal(), 2);
    
    CHECK_EQ(mtnf_.getField().v_[0], 5);
    CHECK_EQ(mtnf_.getField().v_[1], 6);
    CHECK_EQ(mtnf_.getPtr()->v_[0], 5);
    CHECK_EQ(mtnf_.getPtr()->v_[1], 6);
    return;
  }
 private:
  struct NonFundamental {
    NonFundamental& operator+(const NonFundamental& other) {
      for (int i=0; i<static_cast<int>(v_.size()); ++i)
        v_[i] += other.v_[i]; 
      return *this;
    }
    array<int, 4> v_;
  };
  using Fundamental=int;
  
  Fundamental               fundamental_;
  NonFundamental            nfundamental_;
  MetaClass<Fundamental>    mtf_;
  MetaClass<NonFundamental> mtnf_;
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  MetaTester mt;
  mt.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
