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
#include <algorithm>        // std::equal
#include <bitset>           // std::bitset<CHAR_BIT>
#include <sstream>          // std::stringstream
// Standard C Headers
#include <cctype>
// Google Headers
// Local Headers
#include "utils/ds/string_prefix.h"

using namespace std;

namespace asarcar { namespace utils { namespace ds {
//-----------------------------------------------------------------------------
void StringPrefix::resize(int len) { 
  DCHECK(len >= 0);
  size_t sz = ArraySize(len); 
  last_ = GetLast(len);
  v_.resize(sz); 
  // Mask out all bits not relevant
  if (sz > 0)
    v_.at(sz-1) &= GetMask(last_); 
}

StringPrefix StringPrefix::substr(int begin, int runlen) const {
  DCHECK(begin >= 0);
  DCHECK_GE(runlen, 0); DCHECK_LE(begin + runlen, size());
  if (runlen == 0)
    return StringPrefix{""};

  // copy the bits from begin to end of byte to v.
  int old_last = (begin+runlen-1)%CHAR_BIT;
  int idx_first = ByteIndex(begin+1);
  int idx_last  = ByteIndex(begin+runlen);
  char ch = v_.at(idx_first);

  int     beginbitoffset = begin%CHAR_BIT;
  int     runlenbits = min(runlen, CHAR_BIT - beginbitoffset);

  if (idx_last == idx_first) {
    uint8_t mask = GetMask(beginbitoffset, runlenbits);
    char    val  = (ch & mask) << beginbitoffset;
    return StringPrefix{vector<char>{val}, runlenbits-1};
  }

  // copy the bits from (begin,byte-end] to vector
  // invoke CopyBits to copy rest of bits
  vector<char> v;
  uint8_t mask = GetMask(beginbitoffset, runlenbits);
  v.push_back((ch & mask) << beginbitoffset);
  int last = CopyBits(&v, runlenbits-1, v_, old_last, 
                      idx_first + 1, (idx_last - idx_first));
  return StringPrefix{move(v), last}; 
}

StringPrefix& StringPrefix::operator +=(const StringPrefix& other) {
  // result = bits of v_ prepended to bits of other.v
  last_ = CopyBits(&v_, last_, other.v_, other.last_, 0, other.v_.size());
  return *this;
}

StringPrefix StringPrefix::prefix(const StringPrefix& other) const {
  vector<char> v;
  int siz = v_.size();
  int osiz = other.v_.size();
  int i;

  for(i=0; i<siz; ++i) {
    if (i >= osiz)
      break;
    char ch = v_.at(i);
    char ch2 = other.v_.at(i);
    if (ch != ch2)
      break;
    v.push_back(ch);
  }
  
  // other subsumed by this
  if ((i == siz) && (i < osiz))
    return StringPrefix{move(v), last_};

  // this subsumed by other
  if ((i < siz) && (i == osiz))
    return StringPrefix{move(v), other.last_};
           
  if ((i == siz) && (i == osiz))
    return StringPrefix{move(v), min(last_, other.last_)};

  // CASES PENDING: 
  // a. siz == 0 || osiz == 0 => i == siz or i == osiz
  // b. i < siz && i < osiz && siz > 0 && osiz > 0 
  char ch  = v_.at(i);
  char ch2 = other.v_.at(i);
  DCHECK(ch != ch2);

  // identify the mismatch position
  // last is one less than the mismatch position
  uint8_t mask = 0x80;
  int     mismatch;
  int     max1 = (i == siz - 1) ? (last_ + 1) : CHAR_BIT;
  int     max2 = (i == osiz - 1)? (other.last_ + 1) : CHAR_BIT;

  for (mismatch=0; mismatch<max1; ++mismatch) {
    if (mismatch >= max2)
      break;
    if ((ch & mask) != (ch2 & mask))
      break;
    mask >>= 1;
  }

  // we should hit mismatch before all bits are examined
  DCHECK_LT(mismatch, CHAR_BIT); 

  // mismatch character is right from first msb
  // all matched characters are already in vector
  // last = ALL_BYTE in this case
  if (mismatch == 0)
    return StringPrefix{move(v), ALL_BYTE};
    
  mask = GetMask(mismatch-1);
  v.push_back(ch & mask);
  return StringPrefix{move(v), mismatch - 1};
}

std::string StringPrefix::to_string(bool debug) const {
  string str = "", bitsetstr="bits:";
  string dbgstr = "";
  if (debug) {
    dbgstr += "[";
    dbgstr += "size=" + std::to_string(size());
    dbgstr += ": vector_size=" + std::to_string(v_.size());
    dbgstr += ": last_=" + std::to_string(last_) + ": char-sequence=";
    dbgstr += "]";
  }
  int siz = size();
  string end = "/" + std::to_string(siz);
  int len=0;
  std::for_each(v_.begin(), v_.end(), 
                [&len, &str, &bitsetstr, debug, siz](char ch) {
      // print character only if full byte is contained in prefix length
      len += CHAR_BIT;
      bool printable = (isprint(ch) && (len <= siz));
      string bstr = bitset<CHAR_BIT>(ch).to_string();
      string cstr = printable ? string(1, ch) : "#";
      string separator = (len < siz) ? ":" : "";
      str += cstr;
      bitsetstr += bstr + separator;
    });
  return dbgstr + "\"" + bitsetstr + "=" + str + "\"" + end;
}

int StringPrefix::CopyBits(vector<char>*        dst_v_p, 
                           const int            cur_last,
                           const vector<char>&  src_v,
                           const int            src_last,
                           const int            src_offset,
                           const int            src_num_bytes) {
  DCHECK(dst_v_p != nullptr);
  DCHECK_LE(src_offset + src_num_bytes, src_v.size());
  if (dst_v_p->size() == 0) {
    *dst_v_p = src_v;
    return src_last;
  }

  // dst_v_p->size > 0

  // src_v bytes are zero
  if (src_num_bytes == 0)
    return cur_last;
  
  // deal with easy case: full character boundaries
  if (cur_last == CHAR_BIT - 1) {
    dst_v_p->insert(dst_v_p->end(), 
                    src_v.begin()+src_offset, 
                    src_v.begin()+src_offset+src_num_bytes);
    // mask out the non-relevant bits
    dst_v_p->back() &= GetMask(src_last);
    return src_last;
  }

  // worse case: partial character boundary in *this
  int     numbitsmask1 = cur_last + 1;
  int     numrem1 = (CHAR_BIT - cur_last - 1);
  int     bitoffset1 = numrem1;
  uint8_t mask1 = GetMask(bitoffset1, numbitsmask1);
  uint8_t mask2 = GetMask(numrem1);
  uint8_t mask3 = GetMask(src_last);
  int     last  = (numbitsmask1+src_last)%CHAR_BIT;
  uint8_t mask4 = GetMask(bitoffset1, last+1);

  // Iterate till the second last element 
  for (auto it = src_v.begin()+src_offset; 
       it != src_v.begin()+src_offset+src_num_bytes-1; ++it) {
    // update last symbol of this->v_ with its current valid bits and
    // borrow remaining bits from src_v
    dst_v_p->back() |= (*it & mask2) >> numbitsmask1;
    dst_v_p->push_back((*it & mask1) << numrem1);
  }

  // Bits of last character of src_v has to be merged with the valid bits
  // of the last symbol of *this.
#if 0
  DLOG(INFO) << "cur_last=" << cur_last << ": last=" << last
             << std::hex << ": src_v.back()=" << src_v.back()
             << ": mask2=" << (src_v.back() & mask2)
             << ": mask3=" << (src_v.back() & mask3)
             << ": mask4=" << (src_v.back() & mask4)
             << ": numbitsmask=" << numbitsmask1
             << ": numrem1=" << numrem1
             << ": dst_v_p->size()=" << dst_v_p->size()
             << ": dst_v_p->back()=" << dst_v_p->back()
             << ": (src_v.back() & mask3) >> numbitsmask1=" 
             << ((src_v.back() & mask3) >> numbitsmask1)
             << ": (src_v.back() & mask2) >> numbitsmask1=" 
             << ((src_v.back() & mask2) >> numbitsmask1)
             << ": (src_v.back() & mask2) >> numbitsmask1="
             << ((src_v.back() & mask4) << numrem1);
#endif      
  char ch = src_v.at(src_offset+src_num_bytes-1);
  // 1. If the bits are not enough to complement the valid bits of the last
  // symbol of v_ we just borrow whatever is valid and update symbol
  if (cur_last < last) {
    dst_v_p->back() |= (ch & mask3) >> numbitsmask1;
    return last;
  }

  // 2. Total bits spill over to the next symbol
  dst_v_p->back() |= (ch & mask2) >> numbitsmask1;
  dst_v_p->push_back((ch & mask4) << numrem1);
  return last;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace ds {


