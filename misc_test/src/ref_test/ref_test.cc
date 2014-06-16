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
#include <vector>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

int main(int argc, char *argv[]) {
  asarcar::Init::InitEnv(&argc, &argv);

  const std::vector<uint32_t> v{1, 2, 3, 4};
  std::cout << "v: size " << v.size() << ": data " 
            << std::hex << v.data() << std::endl;
  const std::vector<uint32_t> &v1_ref(v);
  std::cout << "v1_ref: size " << v1_ref.size() 
            << ": data " << std::hex << v1_ref.data() << std::endl;
  const std::vector<uint32_t> &v2_ref{v1_ref};
  std::cout << "v2_ref: size " << v2_ref.size() 
            << ": data " << std::hex << v2_ref.data() << std::endl;
  return 0;
}
