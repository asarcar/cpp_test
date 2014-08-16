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
#include "utils/basic/user_defined_units.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);
  
  auto distance     = 8.0_m;
  auto mass         = 2.0_kg;
  auto time         = 2.0_s;
  auto speed        = distance/time;
  auto acceleration = speed/time;
  auto force        = mass*acceleration;
  auto energy       = force*distance;
  CHECK_EQ(speed,        Quantity<MpS>{4.0});
  CHECK_EQ(acceleration, Quantity<Acc>{2.0});
  CHECK_EQ(force,        Quantity<F>{4.0});
  CHECK_EQ(energy,       Quantity<E>{32.0});

  LOG(INFO) << "Distance:     " << distance      << endl
            << "Mass:         " << mass          << endl
            << "Time:         " << time          << endl
            << "Speed:        " << speed         << endl
            << "Acceleration: " << acceleration  << endl
            << "Force:        " << force         << endl
            << "Energy:       " << energy;

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
