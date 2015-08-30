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

//! @file     ipv4.h
//! @brief    Thin wrapper over IPv4 Address.
//! @details  Wrapper for IPv4 with utility APIs. 
//!           NOT internally synchronized for Thread Safety.

//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_NWK_IPV4_PREFIX_H_
#define _UTILS_NWK_IPV4_PREFIX_H_

// C++ Standard Headers
#include <string>       // to_string(int)
// C Standard Headers
#include <cstdlib>
// Google Headers
#include <glog/logging.h>
// Local Headers
#include "utils/nwk/ipv4.h"

namespace asarcar { namespace utils { namespace nwk {
//-----------------------------------------------------------------------------
class IPv4Prefix {
 private:
  static uint32_t Mask(int runlen) {
    return (((1 << runlen) - 1) << (IPv4::MAX_LEN - runlen));
  }

 public:
  IPv4Prefix(uint32_t addr=0, int len=0) : ip_{addr & Mask(len)}, len_{len} {
    DCHECK(len_ <= IPv4::MAX_LEN);
  }
  IPv4Prefix(IPv4 ip, int len = 0): IPv4Prefix{ip.to_scalar(), len} {}
  IPv4Prefix(const char* s, int len) : IPv4Prefix{IPv4{s}, len} {}
  IPv4Prefix(std::string str, int len) : IPv4Prefix{IPv4{str}, len} {}

  // Destructor and other ctors and = on both lvalue and rvalue reference

  inline int size(void) { return len_; }

  inline std::string to_string(void) { 
    return ip_.to_string() + "/" + std::to_string(len_);
  }

  // Substr operator returns an equivalent IPv4 Prefix address but only 
  // with bits in the [begin, begin + len) range.
  IPv4Prefix substr(int begin, int runlen) const;

  // Equality Check Operators
  inline bool operator ==(const IPv4Prefix& other) const {
    return ((ip_ == other.ip_) && (len_ == other.len_));
  }
  inline bool operator !=(const IPv4Prefix& other) const {
    return !operator==(other);
  }

  // Concatenates the bit strings of IPv4 address in the class to the
  // one passed in argument
  IPv4Prefix& operator +=(const IPv4Prefix& other);

  // Concatenates the bit strings of host prefix class to the
  // one passed in argument and creates a new prefix string
  inline IPv4Prefix operator +(const IPv4Prefix& other) {
    return IPv4Prefix{*this}.operator+=(other);
  }

 private:
  IPv4  ip_;
  int   len_;
};



//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace nwk {

#endif // _UTILS_NWK_IPV4_PREFIX_H_
