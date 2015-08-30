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

#ifndef _UTILS_NWK_IPV4_H_
#define _UTILS_NWK_IPV4_H_

// C++ Standard Headers
#include <iostream>
// C Standard Headers
// Google Headers
#include <glog/logging.h>
// Local Headers
#include "utils/basic/basictypes.h"

namespace asarcar { namespace utils { namespace nwk {
//-----------------------------------------------------------------------------
class IPv4 {
 public:
  constexpr static uint32_t LOOPBACK_SUBNET = 0x7F000000; // 127.0.0.0
  constexpr static int      MAX_LEN         = 32; // lenght of v4 bit stream

  // address passed in host endian order
  explicit IPv4(uint32_t addr=0) : addr_{addr} {}
  explicit IPv4(const char* str);
  // Pass rvalue reference for efficiency
  explicit IPv4(std::string str) : IPv4{str.c_str()} {}
  // Destructor and other ctors and = on both lvalue and rvalue reference
  
  // Equality Check Operators
  inline bool operator ==(const IPv4& other) const {
    return (to_scalar() == other.to_scalar());
  }
  inline bool operator !=(const IPv4& other) const {
    return !operator==(other);
  }

  // LoopBack Interface
  inline bool IsLoopbackAddress(void) const {
    return ((addr_ & 0xff000000) == LOOPBACK_SUBNET);
  }
  
  // cast operator to uint32_t
  inline uint32_t to_scalar(void) const { return addr_; }

  // cast operator to string
  std::string to_string(void) const;

  // Returns true if subnet of the passed str_addr matches the subnet
  // of the v4 address given the netmask passed 
  bool SameSubnet(const std::string& str, 
                  const std::string& mask = "255.255.255.255") const;

 private:
  uint32_t addr_; // stored in host endian order
};
//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace nwk {

#endif // _UTILS_NWK_IPV4_H_

