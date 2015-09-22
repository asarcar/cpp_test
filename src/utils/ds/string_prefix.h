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

//! @file     string_prefix.h
//! @brief    Prefix class built for string. 
//! @details  Useful construct used to store string in prefix ordered trie. 
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_STRING_PREFIX_H_
#define _UTILS_DS_STRING_PREFIX_H_

// C++ Standard Headers
#include <iostream>
#include <vector>
// C Standard Headers
#include <climits>          // CHAR_BIT
// Google Headers
#include <glog/logging.h>   // DCHECK
// Local Headers
#include "utils/basic/basictypes.h"

//! @addtogroup ds
//! @{

namespace asarcar { namespace utils { namespace ds {
//-----------------------------------------------------------------------------
class StringPrefix {
 public:
  static constexpr int ALL_BYTE = CHAR_BIT - 1;
  using vchar=std::vector<char>;
  StringPrefix(vchar&& v, int last): 
      last_{last}, v_{std::move(v)} {
    DCHECK(((v_.size() > 0) && (last_ >= 0)) || 
           ((v_.size() == 0) && (last_ == ALL_BYTE))) 
        << "v_.size()=" << v_.size() << ": last_=" << last_;
  }
  StringPrefix(std::string&& str): 
      StringPrefix{vchar(str.begin(), str.end()), ALL_BYTE} {}

  StringPrefix(const char str_p[]): 
      StringPrefix{std::string(str_p)} {}

  inline size_t size(void) const { 
    return (v_.size()*CHAR_BIT + last_ + 1 - CHAR_BIT); 
  }
  void resize(int len); 
  StringPrefix  substr(int begin, int runlen) const;
  inline bool operator ==(const StringPrefix& other) const {
    return ((size() == other.size()) &&
            std::equal(v_.begin(), v_.end(), other.v_.begin()));
  }
  inline bool operator !=(const StringPrefix& other) const {
    return !this->operator==(other);
  }
  
  // True when 'n'th (0 < n < prefix length) most significant bit is 1
  bool operator [](int n) const {
    DCHECK_GE(n, 0); DCHECK_LT(n, size());
    return ((v_.at(ByteIndex(n+1)) & ExtractBitNum(GetLast(n+1))) != 0);
  }

  // Returns new string after concatenation of this and other 
  inline StringPrefix operator +(const StringPrefix& other) const {
    return StringPrefix{*this}.operator+=(other);
  }

  // Appends other string to this: return this
  StringPrefix& operator +=(const StringPrefix& other);

  // Returns the common prefix substring as compared to argument
  StringPrefix prefix(const StringPrefix& other) const;

  std::string to_string(bool debug=false) const;

 private:
  // #bit offset of last byte that is part of symbol
  int    last_;  
  vchar  v_;

  static inline size_t ArraySize(int len) {
    return (len + CHAR_BIT - 1)/CHAR_BIT; // Ceiling(len/CHAR_BIT)
  }
  static inline size_t ByteIndex(int len) {
    return ((len+CHAR_BIT-1)/CHAR_BIT - 1);
  }
  static inline size_t GetLast(int len) {
    return (len + CHAR_BIT - 1)%CHAR_BIT;
  }
  // [begin, runlen) range
  static inline uint8_t GetMask(int begin, int runlen) {
    DCHECK_GE(begin, 0); DCHECK_LT(begin, CHAR_BIT);
    DCHECK_GE(runlen, 0); DCHECK_LE(runlen, CHAR_BIT); 
    return (0xFF << (CHAR_BIT - runlen)) >> begin;
  }
  static inline uint8_t GetMask(int last_len) {
    return GetMask(0, last_len+1);
  }
  static inline uint8_t ExtractBitNum(int last_len) {
    return 0x80 >> last_len;
  }
  static int CopyBits(vchar                *dst_v_p, 
                      const int            cur_last,
                      const vchar&         src_v,
                      const int            src_last,
                      const int            src_offset,
                      const int            src_num_bytes);
};

inline std::ostream& operator << (std::ostream& os, const StringPrefix& pref) {
  os << pref.to_string(false); 
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace ds {

#endif // _UTILS_DS_STRING_PREFIX_H_
