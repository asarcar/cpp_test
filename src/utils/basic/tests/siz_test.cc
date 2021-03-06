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
#include <exception>   // std::throw
#include <iostream>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  int128_t i = 0;
  intptr_t j = 0;

  LOG(INFO) << "sizeof(int128_t) = " << sizeof(i) 
            << ": sizeof(intptr_t) = " << sizeof(j)
            << ": sizeof(long long int) = " << sizeof(long long int)
            << std::endl;

  CHECK_EQ(sizeof(i),16) << "sizeof(int128_t) is " << sizeof(i) << " != 16";
  CHECK_EQ(sizeof(j),sizeof(long int)) 
      << "sizeof(intptr) is " << sizeof(j) 
      << " != sizeof(long int) is " << sizeof(long int);

  LOG(INFO) << "sizeof(string{}) = " << sizeof(string{})
            << ": sizeof(string{\"\"}) = " << sizeof(string{""})
            << ": sizeof(string{\"a\"}) = " << sizeof(string{"a"});

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
