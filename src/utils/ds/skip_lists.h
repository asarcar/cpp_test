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
//! @detail AVL or Red Black Trees create balanced trees that bounds:
//!         - Find/Modify (Insert or Delete) to log(n). 
//!         - FindNext: log(n) time. 
//!         - Memory: Per node is O(N+2x) where
//!           2x is % of memory consumed by key/value versus 2 words of memory. 
//!         Skip Lists statistically supports 
//!         - Find/Modify: in log(n) and FindNext in O(1).
//!         - Memory: Per node is O(N(1+6x)) where x is % of memory consumed by
//!           key/value versus 1 word of pointer.
//!         - With High Probability (WHP) allows all R/W/M/C operation bounded
//!           by lg(n). This distinguishes it from other stochastic dynamic data 
//!           structures (e.g. BinarySearchTree, Treaps, etc.). The alternative 
//!           also provide good expected computation time for typical operations. 
//!           However, the probability distribution for outliars are not guaranteed.  
//!           For guaranteed bounds, one may also use RedBlackTrees or AVL trees.
//!           However, the constants hidden in those dynamic structures are 
//!           not as good as skip lists (trade-off memory).
//!         - Thread Safety: NOT thread safe. For concurrent R/RW access use
//!           synchronization.
//!         
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_SKIP_LISTS_H_
#define _UTILS_DS_SKIP_LISTS_H_

// C++ Standard Headers
#include <array>            // array
#include <iomanip>          // std::setfill, std::setw, std::left/right, ...
#include <functional>       // std::function
#include <random>           // std::distribution, random engine, ...
#include <utility>          // std::pair
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/fassert.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils {
//-----------------------------------------------------------------------------

//! @fn         Ones
//! @param[in]  N  
//! @returns    # of 1s in Numbers
constexpr uint32_t Ones(const uint32_t N) {
  return ((N==0) ? 0 : (Ones(N & (N-1)) + 1));
}

//! @fn         MsbOnePos = Floor(log2(N)) + 1
//! @param[in]  N
//! @returns    msb position starting with lsb position indexed as 1
constexpr uint32_t MsbOnePos(const uint32_t N) {
  return ((N==0) ? 0 : (MsbOnePos(N >> 1) + 1));
}

//! @fn         TrailingOnes
//! @param[in]  N
//! @returns    Gives the # of contiguous trailing 1s in the number
constexpr uint32_t TrailingOnes(const uint32_t N) {
    return (((N & 0x00000001) == 0) ? 0 : TrailingOnes(N>>1) + 1);
}

// Forward Declarations
template <typename T, uint32_t MaxNum>
class SkipList;

template <typename T, uint32_t MaxNum>
class SkipListCIter;

template <typename T, uint32_t MaxNum>
std::ostream& operator << (std::ostream& os, const SkipList<T,MaxNum>& s);

template <typename T, uint32_t MaxNum>
class SkipList {
 public:
  using SkipListPtr = SkipList<T,MaxNum> *;

  static constexpr uint32_t MaxLevel(void) {
    static_assert(MaxNum <= (1 << 16), 
                  "Maximum entries supported for SkipList limited to 2^16 entries"); 
    return MsbOnePos(MaxNum);
  }

  class Node;
  using NodePtr  = Node*;
  using ValCRef  = const T&;
  using AccFn    = const std::function<uint32_t(const NodePtr)>&;

  class Node {
   public:
    virtual ~Node() {}
    virtual size_t Size(void) const = 0;
    virtual NodePtr& Next(uint32_t index) = 0;
    virtual const NodePtr& Next(uint32_t index) const = 0;
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
    size_t Size(void) const override {
      return _next.size();
    }
    NodePtr& Next(uint32_t index) override {
      return _next.at(index);
    }
    const NodePtr& Next(uint32_t index) const override {
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

  SkipList() : 
    _num_nodes{0}, _head{T{}}, _seed{std::random_device{}()},
    _random{std::bind(std::uniform_int_distribution<uint32_t>
                      // random number between 0 to 2^MaxLevel()-1
                      {0, static_cast<uint32_t>(static_cast<int>(1 >> MaxLevel())-1)}, 
                      std::default_random_engine{_seed})} {} 
  ~SkipList() {
    // Free up memory for all nodes in the linked list
    NodePtr next_np, np = Head()->Next(0);
    while (np != nullptr) {
      next_np = np->Next(0);
      DeleteNode(np);
      np = next_np;
    }
  }
  inline NodePtr  Head(void) { return &_head; }
  inline const SkipList<T,MaxNum>::Node* Head(void) const { return &_head; }
  inline size_t   Size(void) const { return _num_nodes; }


  // Iterator
  friend class SkipListCIter<T, MaxNum>;
  using ItC   = SkipListCIter<T, MaxNum>;
  // type returned for Modify operations Emplace/Insert/Remove
  using ModPr = std::pair<ItC,bool>;

  inline ItC begin(void) {return ItC{this};}
  inline ItC end(void) {return ItC{this, nullptr};}

  // Operations: Find/Insert/Remove...
  ItC Find(const T& val) {
    NodePtr np=Head();
    for(int lvl = static_cast<int>(MaxLevel()) - 1; lvl >= 0; --lvl) {
      for(;np->Next(lvl) != nullptr && np->Next(lvl)->Value() < val; 
          np = np->Next(lvl));
      if (np->Next(lvl) != nullptr && np->Next(lvl)->Value() == val) 
        return ItC{this, np->Next(lvl)};
    }
    return end();
  }

  // Constructs element in place and Inserts to SkipList if Value does not exist.
  // Returns <Iterator,false> if element exists (insert failed) else <iterator, true>
  ModPr Emplace(T&& val) {
    // Cache Nodes at every level whose next is insert node so we can insert if needed
    std::array<NodePtr, MaxLevel()> nodeptrs;
    NodePtr np=Head();

    for(int lvl = static_cast<int>(MaxLevel()) - 1; lvl >= 0; --lvl) {
      for(;np->Next(lvl) != nullptr && np->Next(lvl)->Value() < val; 
          np = np->Next(lvl));
      if ((np->Next(lvl) != nullptr) && (np->Next(lvl)->Value() == val))
        return ModPr{ItC{this, np->Next(lvl)},false};
      // np is the previous node candidate. cache it.
      // i.e. np == head || np->Value < val AND
      //      np->Next(lvl) == nullptr OR np->Next(lvl)->Value > val
      nodeptrs[lvl] = np;
    }

    // create a new node with a random number of levels 
    NodePtr insp = NewNode(std::move(val));
    // link node with the previous nodes in the list
    // at all levels this node should participate in the linked list
    for(int lvl = 0; lvl < static_cast<int>(insp->Size()); ++lvl) {
      NodePtr tmp = nodeptrs[lvl];
      insp->Next(lvl) = tmp->Next(lvl);
      tmp->Next(lvl) = insp;
    }

    return ModPr{ItC{this, insp}, true};
  }

  inline ModPr Insert(T&& val) { return Emplace(std::move(val)); }

  // Removes element and returns the first element after the remove candidate 
  // followed by bool (TRUE) if candidate was indeed removed (when it exists)
  ModPr Remove(const T& val) {
    // Cache Nodes at every level whose next is insert node so we can insert if needed
    std::array<NodePtr, MaxLevel()> nodeptrs;
    NodePtr np=Head();

    for(int lvl = static_cast<int>(MaxLevel()) - 1; lvl >= 0; --lvl) {
      for(;np->Next(lvl) != nullptr && np->Next(lvl)->Value() < val; 
          np = np->Next(lvl));
      // np is the previous node candidate. cache it.
      // i.e. np == head || np->Value < val AND
      //      np->Next(lvl) == nullptr OR np->Next(lvl)->Value >= val
      // np->Next(lvl) == nullptr OR np->Next(lvl)->Value() >= val
      nodeptrs[lvl] = np;
    }

    NodePtr rem=np->Next(0);
    // Candidate Exists i.e. np->Next(0) == val otherwise not
    if (rem == nullptr || rem->Value() != val)
      return ModPr{ItC{this, rem}, false};

    // 1. Remove node from all the previous nodes in the list
    //    at all levels of this node.
    // 2. Delete the node
    for(int lvl = 0; lvl < static_cast<int>(rem->Size()); ++lvl)
      nodeptrs[lvl]->Next(lvl) = rem->Next(lvl);
    ModPr res{ItC{this, rem->Next(0)}, true};
    DeleteNode(rem);
    
    return res;
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

  //! @fn         inorder traverses nodes of treap
  //! @param[in]  function executed on every node 
  //! @param[in]  level at which to traverse the skiplist
  //! @returns    accumulated result
  inline uint32_t 
  InOrder(AccFn fn, uint32_t level) const { 
    uint32_t acc = 0;
    for (NodePtr np = Head()->Next(level); np != nullptr; np = np->Next(level))
      acc += fn(np);
    return acc;
  }

  // helper function to allow chained cout cmds: example
  // cout << "Treap: " << endl << t << endl << "---------" << endl;
  friend std::ostream& operator << <>(std::ostream& os, const SkipList<T,MaxNum>& s);
  
 private:
  uint32_t                      _num_nodes;
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

    _num_nodes++;
    return np;
  }

  inline void DeleteNode(NodePtr node_p) {
    _num_nodes--;
    delete node_p;
    return;
  }

  //! @fn        NodeLevel
  //! @details   Provides a number between 1 and MaxLevel() with a 
  //!            specific probability distribution - refer below.
  //!            Generates a random number and measures the number of 
  //!            contiguous trailing 1s in the generated number.
  //!            This yield a number with probability distribution, such that:
  //!            Level = 1 i.e. default     = 1
  //!            Level = 2 i.e. prob(num=1) = 1/2
  //!            Level = 3 i.e. prob(num=2) = 1/4
  //!            Level = k i.e. prob(num=k) = 1/2^k; ... 
  //! @returns   Level
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
    FASSERT(_cur->Size() > i);
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
  inline NodePtr operator->(void) {
    return this->_cur;
  }
  friend class SkipList<T, MaxNum>;
 private:
  SkipListPtr _sl;
  NodePtr     _cur;
};

// SkipList Output
// helper function to allow chained cout cmds: example
// cout << "SkipList: " << endl << s << endl << "---------" << endl;
template <typename T, uint32_t MaxNum>
std::ostream& operator << (std::ostream& os, const SkipList<T,MaxNum>& s) {
  using NodePtr = typename SkipList<T,MaxNum>::NodePtr;

  os << std::endl; 
  os << "#************************#" << std::endl;
  os << "# SkipList:              #" << std::endl;
  os << "#------------------------#" << std::endl;
  os << "# Size=" << std::setfill(' ') << std::setw(10) 
     << std::setfill(' ') << std::left << s.Size() 
     << "--------#" << std::endl;
  os << "##########################" << std::endl;
  s.InOrder([&os] (NodePtr np) {
      os << np->Value() << ": nextptrs #" << np->Size() << " [";
      for (int j=0; j < static_cast<int>(np->Size()); j++) 
        os << std::hex << " 0x" << np->Next(j) << std::dec;
      os << " ]" << std::endl;
      return 1;
    }, 0);
  os << "#************************#" << std::endl;
  
  return os;
}

//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace utils {

#endif // _UTILS_DS_SKIP_LISTS_H_
