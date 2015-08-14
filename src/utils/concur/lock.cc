// Copyright 2015 asarcar Inc.
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
// Standard C Headers
// Google Headers
// Local Headers
#include "utils/concur/lock.h"

using namespace std;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
constexpr 
array<const char *, static_cast<std::size_t>(LockMode::EXCLUSIVE_LOCK)+1>
Lock::_toStr;
//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

