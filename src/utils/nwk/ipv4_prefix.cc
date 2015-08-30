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
#include "utils/nwk/ipv4_prefix.h"

using namespace std;

namespace asarcar { namespace utils { namespace nwk {
//-----------------------------------------------------------------------------

IPv4Prefix IPv4Prefix::Substr(int begin, int runlen) const {
  DCHECK(begin >= 0);
  DCHECK((runlen > 0) && ((begin + runlen) <= len_));
  uint32_t addr = ip_.to_scalar();
  uint32_t new_mask = Mask(runlen);
  uint32_t new_addr = (addr << begin) & new_mask;
  return IPv4Prefix{new_addr, runlen};
}

IPv4Prefix IPv4Prefix::Join(const IPv4Prefix& other) {
  DCHECK((len_ + other.len_) <= IPv4::MAX_LEN);
  uint32_t addr  = ip_.to_scalar();
  uint32_t addr2 = other.ip_.to_scalar();
  uint32_t new_addr = addr | (addr2 >> len_);
  int      new_len  = len_ + other.len_;
  return IPv4Prefix{new_addr, new_len};
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace nwk {

