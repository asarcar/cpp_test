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

//! @file     make_unique.h
//! @brief    unique counterpart function of make shared
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_MAKE_UNIQUE_H_
#define _UTILS_BASIC_MAKE_UNIQUE_H_

// C++ Standard Headers
#include <iostream>
#include <memory>           // std::unique_ptr
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/meta.h"

//! Generic namespace used for all utility routines developed
namespace asarcar {
//-----------------------------------------------------------------------------

//! @brief    Routine to create a unique point for vanilla struct/class
template <typename T, typename... Args>
static inline EnableIf<!IsArray<T>(), std::unique_ptr<T>> 
make_unique(Args&&... args) {
  return std::unique_ptr<T>{new T{std::forward<Args>(args)...}};
}

//! @brief    Routine to create a unique point for fixed array of struct/class
template <typename T, typename... Args>
static inline 
EnableIf<(IsArray<T>() && !IsArray<RemoveExtent<T>>()), std::unique_ptr<T>> 
make_unique(Args&&... args) {
  return std::unique_ptr<T>{new RemoveExtent<T>[sizeof...(args)]{std::forward<Args>(args)...}};
}


//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_MAKE_UNIQUE_H_
