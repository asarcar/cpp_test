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

//! @file   proc_info.cc
//! @brief  Gets processor related information
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <fstream>          // std::ifstream & std::ofstream
#include <iostream>
#include <sstream>          // std::stringstream
// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/proc_info.h"

using namespace std;

namespace asarcar { 

ProcInfo::ProcInfo() : num_cores_{0}, flags_{} {
  ifstream inp;
  string line;

  // Parse the information by reading /proc/cpuinfo
  inp.open("/proc/cpuinfo", std::ios::in);
  FASSERT(inp); 

  while (getline(inp, line)) {
    // # cores: # of lines beginning with processor 
    if (line.find("processor") == 0) {
      num_cores_++;
      continue;
    } else if (line.find("flags") != 0) {
        continue;
    }
    // line begins with "flags": skip over till we find ":"
    size_t pos = line.find(':');
    line.erase(0, pos+1); // skip over till we skip over ':' character

    std::string token;
    while ((pos = line.find(' ')) != std::string::npos) {
      token = line.substr(0, pos);
      if (!token.empty())
        flags_.insert(token);
      line.erase(0, pos+1);
    }
  }
  return;
} 

} // namespace asarcar


