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

//! @file   skip_lists.h
//! @brief  Linked List for write/modify heavy workloads
//! @detail AVL or Red Black Trees create balanced trees that bound the
//!         the Find/Modify (Insert or Delete) to log(n). 
//!         FindNext takes log(n) time. Memory per node is O(N+2x) where
//!         2x is % of memory consumed by key/value versus 2 words of memory. 
//!         Skip Lists support Find/Modify in log(n) and FindNext in O(1).
//!         Memory per node is O(N(1+6x)) where x is % of memory consumed by
//!         key/value versus 1 word of pointer.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_SKIP_LISTS_H_
#define _UTILS_DS_SKIP_LISTS_H_

// C++ Standard Headers
#include <array>                    // array
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils {
//-----------------------------------------------------------------------------

// @fn         NumOnes
// @param[in]  N  
// @returns    # of 1s in Numbers
constexpr uint32_t NumOnes(const uint32_t N) {
  return ((N==0) ? 0 : (NumOnes(N & (N-1)) + 1));
}

// @fn         MsbPos
// @param[in]  N
// @returns    msb position starting with lsb position indexed as 1
constexpr uint32_t MsbPos(const uint32_t N) {
  return ((N==0) ? 0 : (MsbPos(N >> 1) + 1));
}

// @fn         LogBaseTwoCeiling
// @param[in]  N (assumed N > 0)
// @returns    Ceiling(log(N))
constexpr uint32_t LogBaseTwoCeiling(const uint32_t N) {
  return ((NumOnes(N) <= 1) ? MsbPos(N) : (MsbPos(N) + 1)); 
}

template <typename T, uint32_t MaxNum>
class SkipList {
 public:

  static constexpr uint32_t MaxLevel(void) {
    return LogBaseTwoCeiling(MaxNum);
  }

  class Node;
  using NodePtrC = Node * const;

  class Node {
   public:
    virtual uint32_t Size(void) const = 0;
    virtual NodePtrC Next(uint32_t index) = 0;
    virtual T& Value(void) = 0;
  };

  template <uint32_t Level>  
  class ValNode : public Node {
   public:
    using NodeArray = std::array<Node*, Level>;

    ValNode(const T&& val) : Node{}, _val{val} {}
    uint32_t Size(void) const override {
      return _next.size();
    }
    NodePtrC Next(uint32_t index) override {
      return _next.at(index);
    }
    T& Value(void) override {
      return _val;
    }
   private:
    T          _val;
    NodeArray  _next;
  };

  SkipList() : _head{T()} {}

  NodePtrC Head(void) {
    return &_head;
  }

 private:
  ValNode<MaxLevel()> _head;
};

//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace utils {

#endif // _UTILS_DS_SKIP_LISTS_H_
