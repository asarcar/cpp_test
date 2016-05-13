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
#include <exception>
#include <iostream>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/misc/defer.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::misc;
using namespace std;

class DeferTester {
 private:
  using Fn = function<int(int *)>;
  class MyException: public exception {
    virtual const char* what() const throw() {
      return "MyException Happened";
    }
  };

 public:
  DeferTester() {
    fn1_ = [](int* p){return ++(*p);};
    fn2_ = [](int* p){return (*p += 2);};
    fn3_ = [](int* p){return (*p *= 10);};
  }
  void SanityTest(void) {
    int val = 1;
    Defer d1(fn1_, &val); // I1
    {
      Defer d2(fn2_, &val); // I2
      {
        Defer d3(fn3_, &val); // I3
        CHECK_EQ(val, 1);
      } // I3 executes: val = 10
      CHECK_EQ(val, 10);
    } // I2 executes: val = 12
    CHECK_EQ(val, 12);
    LOG(INFO) << "SanityTest Passed";
    return;
  }
  void ExceptionTest(void) {
    int val = 1;
    try {
      Defer d1(fn1_, &val); // I1
      CHECK_EQ(val, 1);
      ThrowException(&val);
    }
    catch (exception &e) {
      Defer d2(fn2_, &val); // I2
      CHECK_EQ(val, 11); // I3 -> I1
      LOG(INFO) << e.what();
    }
    LOG(INFO) << "ExceptionTest Passed";
    return;
  }
 private:
  Fn fn1_, fn2_, fn3_; 
  void ThrowException(int *p) throw (MyException) {
    int v = *p;
    Defer d3(fn3_, p); // I3
    CHECK_EQ(*p, v);
    CHECK_EQ(*p, 1);
    throw MyException{};
  }
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  LOG(INFO) << argv[0] << " Executing Test";
  DeferTester dt;
  dt.SanityTest();
  dt.ExceptionTest();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
