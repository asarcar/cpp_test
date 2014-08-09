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
#include "utils/basic/basictypes.h"

#include "template.h"

template <typename X>
Elem<X>::Elem(void): val_{} {}

template <typename X>
Elem<X>::Elem(const X& val): val_{val}{}

template <typename X>
const X& Elem<X>::Get(void) { return val_; }

template <typename X>
void Elem<X>::Dump(void) {std::cout << "Elem: value=" << val_ << std::endl;}

// Trigger instantiation of Elem<Class>
template class Elem<int>;
template class Elem<double>;


