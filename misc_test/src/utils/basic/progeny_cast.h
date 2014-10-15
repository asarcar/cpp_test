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

//! @file     progeny_cast.h
//! @brief    Provides efficient (non RTTI) away to cast to progeny instances
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_PROGENY_CAST_H_
#define _UTILS_BASIC_PROGENY_CAST_H_

// C++ Standard Headers
#include <iostream>
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/meta_utils.h"

namespace asarcar {
//-----------------------------------------------------------------------------

//! @brief  Progeny Cast: 
//! @detail Dynamic Cast incurs overhead of RTTI (Run Time Type Identification) 
//!         at runtime - a C++ specialization type introspection
//!         progeny_cast avoids RTTI: efficient version of dynamic_cast
//!         Note dynamic_cast, base and derived classes must be polymorphic 
//!         and derived must derive base publicly. We are circumventing 
//!         those extra checks via IsBaseOf
template <typename Derived, typename Base>
inline EnableIf<IsBaseOf<Base,Derived>(), Derived*> 
progeny_cast(Base* base_p) { 
#ifdef NDEBUG
  return static_cast<Derived *>(base_p); 
#else
  return dynamic_cast<Derived *>(base_p);
#endif
}



template <typename Derived, typename Base>
inline EnableIf<IsBaseOf<Base,Derived>(), Derived&> 
progeny_cast(Base& base_ref) { 
#ifdef NDEBUG
  return static_cast<Derived&>(base_ref); 
#else
  return dynamic_cast<Derived&>(base_ref);
#endif
}

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_PROGENY_CAST_H_
