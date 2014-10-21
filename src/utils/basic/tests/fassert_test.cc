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
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  int32_t i = 0, j = 1, k = 2;
  if (argc >= 2)
    i = atoi(argv[1]);
  if (argc >= 3)
    j = atoi(argv[2]);
  if (argc >= 4)
    k = atoi(argv[3]);

  LOG(INFO) << "FAssert State: CurMode " << static_cast<uint32_t>(FAssert::_cur_mode) 
            << ": Def_Level " << static_cast<uint32_t>(FAssert::_def_level)
            << ": Cur_Level " << static_cast<uint32_t>(FAssert::_cur_level)
            << ": FASSERT_REACTION_MODE " << FASSERT_REACTION_MODE
            << ": FASSERT_REACTION_LEVEL " << FASSERT_REACTION_LEVEL
            << std::endl;

  FAssert::dynamic_assert<FAssert::honor_level(FAssert::ReactionLevel::_fatal), 
                          FAssert::Error>(i==0, "(i == 0) condition check blew up!");
  CHECK_EQ(i, 0) << "i == " << i << ": unless 0 we should have fasserted";
  FASSERT_LVL_MSG(FAssert::ReactionLevel::_error, j==1, "j condition blew up badly!!");
  CHECK_EQ(j, 1) << "j == " << j << ": unless 1 we should have fasserted";
  FASSERT(k==2);
  CHECK_EQ(k, 2) << "j == " << k << ": unless 2 we should have fasserted";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
