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

//! @file   init.h
//! @brief  Interface: init environment, enable logging, parse flags, etc.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_INIT_H_
#define _UTILS_BASIC_INIT_H_

// C++ Standard Headers
#include <iostream>         // std::cout
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup utils
//! @{

namespace asarcar {
//-----------------------------------------------------------------------------

//! @class    Init
//! @brief    Init basic services: logging, int handling, option parsing, ...
class Init {
 public:
  Init() = delete;
  static void InitEnv(int *argc_p, char **argv_p[]);
  static std::string GetEnvStr(const char *env_name);
 protected:
 private:
};

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_INIT_H_


