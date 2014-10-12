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
#include <initializer_list>
#include <iostream>
#include <unordered_set>      // std::unordered_set
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/misc/int128_templates.h"
#include "utils/misc/range_bindor.h"
#include "utils/misc/meta.h"
#include "utils/misc/progeny_cast.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::misc;
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
    mtf_{fundamental_}, mtnf_{nfundamental_}, 
    mul_{
      [](std::pair<int, int> v1, int a, int b, std::pair<int,int> v2, int c){
        return v1.first*v1.second*a*b*v2.first*v2.second*c;
      }
    }, set{numeric_limits<int128_t>::min(), numeric_limits<int128_t>::max()} {} 
  ~MetaTester(void) {}
  void Run(void) {
    MetaClassTest();
    FnBindTest();
    Int128Test();
    ProgenyCastTest();
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

  using IntPair=std::pair<int,int>;
  // Bind Arguments to Positions
  template<typename Ret, typename... Args, int... Positions>
  std::function<Ret(Args...)> 
  fn_bind_arg_pos(std::function<Ret(Args...)>& fn_orig,
                  const Range<Positions...>&   arg_pos) {
    return std::bind(fn_orig, std::_Placeholder<Positions>()...);
  }

  template <typename Ret, typename... Args>
  std::function<Ret(Args...)>
  fn_bind(std::function<Ret(Args...)>& fn_orig) {
    auto range = BasicRangeBindor<sizeof...(Args)>();
    return fn_bind_arg_pos(fn_orig, range);
  }

  function<int(IntPair,int,int,IntPair,int)>   mul_;

  unordered_set<int128_t> set;

  struct B {virtual ~B(){};};
  struct D: public B {};
  struct D2: public B {};


  void MetaClassTest(void) {
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
  void FnBindTest(void) {  
    LOG(INFO) << "FN_BIND: Test";
    auto mul2 = fn_bind(mul_);
    CHECK_EQ(mul2(std::make_pair(2,4),6,8,std::make_pair(10,12),14), (2*4*6*8*10*12*14));
    return;
  }

  void Int128Test(void) {
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

  void ProgenyCastTest(void) {
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
