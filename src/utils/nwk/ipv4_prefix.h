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
 public:
  IPv4Prefix(uint32_t addr=0, int len=0) :ip_{addr & GetMask(len)},len_{len} {
    DCHECK(len_ >= 0 && len_ <= IPv4::MAX_LEN);
  }
  IPv4Prefix(IPv4 ip, int len = 0): IPv4Prefix{ip.to_scalar(), len} {}
  IPv4Prefix(const char* s, int len) : IPv4Prefix{IPv4{s}, len} {}
  IPv4Prefix(std::string str, int len) : IPv4Prefix{IPv4{str}, len} {}
  // Destructor and other ctors and = on both lvalue and rvalue reference

  inline size_t size(void) const { return len_; }

  // We only support resizing to smaller values
  inline void resize(int len) { 
    DCHECK(len <= len_);
    ip_.resize(ip_.to_scalar() & GetMask(len));
    len_ = len;
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

  // True when 'n'th (0 < n < prefix length) most significant bit is 1
  inline bool operator [](int n) const {
    DCHECK_GE(n, 0); DCHECK_LT(n, len_);
    return ip_[n];
  }

  // Concatenates the bit strings of host prefix class to the
  // one passed in argument and creates a new prefix string
  inline IPv4Prefix operator +(const IPv4Prefix& other) const {
    return IPv4Prefix{*this}.operator+=(other);
  }

  // Concatenates other to this and returns this
  IPv4Prefix& operator +=(const IPv4Prefix& other);

  // Returns the common prefix substring as compared to argument
  IPv4Prefix prefix(const IPv4Prefix& other) const;

  inline std::string to_string(bool debug=false) const { 
    return ip_.to_string() + "/" + std::to_string(len_);
  }

 private:
  IPv4  ip_;
  int   len_;
  static inline uint32_t GetMask(int runlen) {
    DCHECK(runlen <= IPv4::MAX_LEN);
    return 0xFFFFFFFF << (IPv4::MAX_LEN - runlen);
  }
};

inline std::ostream& operator << (std::ostream& os, const IPv4Prefix& pref) {
  os << pref.to_string(); 
  return os;
}
//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace nwk {

#endif // _UTILS_NWK_IPV4_PREFIX_H_
