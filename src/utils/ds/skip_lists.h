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
#include <array>            // array
#include <functional>       // std::function
#include <random>           // std::distribution, random engine, ...
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/fassert.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils {
//-----------------------------------------------------------------------------

// @fn         Ones
// @param[in]  N  
// @returns    # of 1s in Numbers
constexpr uint32_t Ones(const uint32_t N) {
  return ((N==0) ? 0 : (Ones(N & (N-1)) + 1));
}

// @fn         MsbOnePos = Floor(log2(N)) + 1
// @param[in]  N
// @returns    msb position starting with lsb position indexed as 1
constexpr uint32_t MsbOnePos(const uint32_t N) {
  return ((N==0) ? 0 : (MsbOnePos(N >> 1) + 1));
}

// @fn         TrailingOnes
// @param[in]  N
// @returns    Gives the # of contiguous trailing 1s in the number
constexpr uint32_t TrailingOnes(const uint32_t N) {
    return (((N & 0x00000001) == 0) ? 0 : TrailingOnes(N>>1) + 1);
}

// Forward Declaration
template <typename T, uint32_t MaxNum>
class SkipListCIter;

template <typename T, uint32_t MaxNum>
class SkipList {
 public:
  using SkipListPtr=SkipList<T,MaxNum> *;

  static constexpr uint32_t MaxLevel(void) {
    static_assert(MaxNum <= (1 << 16), 
                  "Maximum entries supported for SkipList limited to 2^16 entries"); 
    return MsbOnePos(MaxNum);
  }

  class Node;
  using NodePtr = Node*;
  using ValCRef  = const T&;

  class Node {
   public:
    virtual ~Node() {}
    virtual uint32_t Size(void) const = 0;
    virtual NodePtr& Next(uint32_t index) = 0;
    virtual ValCRef  Value(void) const = 0;
  };

  template <uint32_t Level>  
  class ValNode : public Node {
   public:
    using ValNodePtr = ValNode *;
    // Value Initialization of Array _next
    ValNode(T&& val) : Node{}, _val{std::move(val)}, _next{} {}
    ValNode(const T& val) : Node{}, _val{val} {}
    
    // Override Methods
    uint32_t Size(void) const override {
      return _next.size();
    }
    NodePtr& Next(uint32_t index) override {
      return _next.at(index);
    }
    ValCRef Value(void) const override {
      return _val;
    }

   private:
    using NodePtrArray = std::array<NodePtr, Level>;

    T             _val;
    NodePtrArray  _next;
  };

  SkipList() : _head{T{}}, _seed{std::random_device{}()},
    _random{std::bind(std::uniform_int_distribution<uint32_t>
                      {0, MaxLevel()}, 
                      std::default_random_engine{_seed})} {} 
  ~SkipList() {
    // Forward Iterate through all nodes and free up resources
  }
  inline NodePtr Head(void) {return &_head;}

  // Iterator
  friend class SkipListCIter<T, MaxNum>;
  using ItC = SkipListCIter<T, MaxNum>;

  inline ItC begin(void) {return ItC{this};}
  inline ItC end(void) {return ItC{this, nullptr};}

  // Operations: Find/Insert/Remove...
  ItC Find(const T& val) {
    NodePtr np=Head();
    for(int level = static_cast<int>(MaxLevel()) - 1; level >= 0; --level) {
      uint32_t lvl = static_cast<uint32_t>(level);
      for(;np->Next(lvl) != nullptr && np->Next(lvl)->Value() < val; 
          np = np->Next(lvl));
      if (np->Next(lvl) != nullptr && np->Next(lvl)->Value() == val) 
        return ItC{this, np->Next(lvl)};
    }
    return end();
  }

  // Inserts element to SkipList if Node with Value does not exist.
  // Returns Iterator to the element (if its exists to the new node else to existing node)
  ItC Insert(T&& val) {
    // Cache Nodes at every level whose next is insert node so we can insert if needed
    std::array<NodePtr, MaxLevel()> nodeptrs;
    NodePtr np=Head();

    for(int level = static_cast<int>(MaxLevel()) - 1; level >= 0; --level) {
      uint32_t lvl = static_cast<uint32_t>(level);
      for(;np->Next(lvl) != nullptr && np->Next(lvl)->Value() < val; 
          np = np->Next(lvl));
      if ((np->Next(lvl) != nullptr) && (np->Next(lvl)->Value() == val))
        return ItC{this, np->Next(lvl)};
      // np is the previous node candidate. cache it.
      // i.e. np == head || np->Value < val AND
      //      np->Next(lvl) == nullptr OR np->Next(lvl)->Value > val
      nodeptrs[lvl] = np;
    }

    // create a new node with a random number of levels 
    NodePtr insp = NewNode(std::move(val));
    // link node with the previous nodes in the list
    // at all levels this node should participate in the linked list
    for(uint32_t lev = 0; lev < insp->Size(); ++lev) {
      NodePtr tmp = nodeptrs[lev];
      insp->Next(lev) = tmp->Next(lev);
      tmp->Next(lev) = insp;
    }

    return ItC{this, insp};
  }

  // For repeatable & predictable node level generation, we 
  // generate the same seeds for random number when testing 
  // such that same random number is generated every time
  void SetPredictableNodeLevel(void) {
    _seed = kFixedCostSeedForRandomEngine;
    _random = std::bind(std::uniform_int_distribution<uint32_t>
                        {0, MaxLevel()}, 
                        std::default_random_engine{_seed}); 
  }
 private:
  ValNode<MaxLevel()>           _head;
  uint32_t                      _seed;
  std::function<uint32_t(void)> _random;
                         
  //! Fixed seed generates predictable MC runs when running test SW or debugging
  const static uint32_t kFixedCostSeedForRandomEngine = 13607; 

  NodePtr NewNode(T&& val) {
    uint32_t level = NodeLevel();
    
    FASSERT(level >= 1 && level <= MaxLevel());
    NodePtr np;

    switch(level) {
      case 1 : np = new ValNode<1>{std::move(val)}; break;
      case 2 : np = new ValNode<2>{std::move(val)}; break;
      case 3 : np = new ValNode<3>{std::move(val)}; break;
      case 4 : np = new ValNode<4>{std::move(val)}; break;
      case 5 : np = new ValNode<5>{std::move(val)}; break;
      case 6 : np = new ValNode<6>{std::move(val)}; break;
      case 7 : np = new ValNode<7>{std::move(val)}; break;
      case 8 : np = new ValNode<8>{std::move(val)}; break;
      case 9 : np = new ValNode<9>{std::move(val)}; break;
      case 10: np = new ValNode<10>{std::move(val)}; break;
      case 11: np = new ValNode<11>{std::move(val)}; break;
      case 12: np = new ValNode<12>{std::move(val)}; break;
      case 13: np = new ValNode<13>{std::move(val)}; break;
      case 14: np = new ValNode<14>{std::move(val)}; break;
      case 15: np = new ValNode<15>{std::move(val)}; break;
      default: np = new ValNode<16>{std::move(val)}; break;
    }

    return np;
  }

  static inline void DeleteNode(NodePtr node_p) {
    delete node_p;
    return;
  }

  // @fn         NodeLevel
  // @details    Provides a number between 1 and MaxLevel() with a 
  //             specific probability distribution - refer below.
  //             Generates a random number and measures the number of 
  //             contiguous trailing 1s in the generated number.
  //             This yield a number with probability distribution, such that:
  //             Level = 1 i.e. default     = 1
  //             Level = 2 i.e. prob(num=1) = 1/2
  //             Level = 3 i.e. prob(num=2) = 1/4
  //             Level = k i.e. prob(num=k) = 1/2^k; ... 
  // @returns    Level
  uint32_t inline NodeLevel(void) {
    uint32_t num = _random(); 
    uint32_t level = TrailingOnes(num) + 1;
    return (level <= MaxLevel() ? level : MaxLevel());
  }
};

// Forward Iterator for SkipLists. TODO: Should have READ ONLY access 
// to key (i.e. key attribute of T).
template <typename T, uint32_t MaxNum>
class SkipListCIter {
 public:
  using SList       = SkipList<T, MaxNum>;
  using SkipListPtr = typename SList::SkipListPtr;
  using NodePtr     = typename SList::NodePtr;
  using ItC         = typename SList::ItC;
  SkipListCIter(SkipListPtr slp) : 
      _sl{slp}, _cur{slp->Head()->Next(0)} {}
  SkipListCIter(SkipListPtr slp, NodePtr np) : 
      _sl{slp}, _cur{np} {}
  inline const T& operator*() {
    FASSERT(_cur != nullptr);
    return _cur->Value();
  }
  inline ItC& operator++() {
    FASSERT(_cur != nullptr);
    _cur = _cur->Next(0);
    return *this;
  }
  inline ItC& Next(uint32_t i) {
    FASSERT(_cur != nullptr);
    FASSERT(_cur.Size() > i);
    FASSERT(SList::MaxLevel() > i);
    _cur = _cur->Next(i);
    return *this;
  }
  inline bool operator==(const ItC &other) {
    return (this->_sl == other._sl && this->_cur == other._cur);
  }
  inline bool operator!=(const ItC &other) {
    return !this->operator==(other);
  }
 private:
  SkipListPtr _sl;
  NodePtr     _cur;
};

//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace utils {

#endif // _UTILS_DS_SKIP_LISTS_H_
