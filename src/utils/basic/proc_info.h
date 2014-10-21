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

//! @file   proc_info.h
//! @brief  Provides Processor Information (#cores), Flags, etc.
//!         # cores are also available via std::thread::hardware_concurrency()
//!         However this class also exposes processor flags and other information
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_PROC_INFO_H_
#define _UTILS_BASIC_PROC_INFO_H_

// C++ Standard Headers
#include <iostream>         
#include <unordered_set>    
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup utils
//! @{

namespace asarcar {
//-----------------------------------------------------------------------------

//! @class    ProcInfo
//! @brief    Provide processor related information of the machine
class ProcInfo {
 public:
  // Meyer's singleton class
  static inline ProcInfo* Singleton(void) {
    static ProcInfo *singleton = new ProcInfo();
    return singleton;
  }
  inline int NumCores(void) const { return num_cores_; }
  inline const std::unordered_set<std::string>& Flags(void) const { return flags_; }
 private:
  ProcInfo(); 
  int num_cores_;
  std::unordered_set<std::string> flags_;
};
//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_PROC_INFO_H_
