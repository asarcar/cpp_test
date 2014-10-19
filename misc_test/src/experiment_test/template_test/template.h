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
  const X& Get(void) const;
  void Dump(void) const;
  constexpr static X kDefaultValue{10};
 private:
  X val_;
};


// Defining an "ordinary" (non-template) class to test whether
// constexpr static ... requires explicit definition outside the class
// if it is referenced elsewhere.
// Note this external definition is not needed in case of templates
// presumably because the "instantiation (elaboration) process" 
// of the template takes care of allocating space that needs referencing.
class Element {
 public:
  explicit Element(void): val_{kDefaultValue} {}
  explicit Element(const float& val): val_{val} {}
  const float& Get(void) const {return val_;}
  void Dump(void) const {std::cout << "Element: value=" << val_ << std::endl;}
  constexpr static float kDefaultValue{1000.0};
 private:
  float val_;
};

// Suppress implicit instantation of Elem
extern template class Elem<int>;
extern template class Elem<double>;

#endif // _TEMPLATE_H_
