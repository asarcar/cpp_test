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
// Standard C Headers
// Google Headers
// Local Headers

//! @file     template.h
//! @brief    Test Header to implement template class
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

template <typename X>
class Elem {
 public:
  explicit Elem(void);
  explicit Elem(const X&);
  const X& Get(void);
  void Dump(void);
 private:
  X val_;
};

// Suppress implicit instantation of Elem
extern template class Elem<int>;

#endif // _TEMPLATE_H_
