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

//! @file   radix_trie.h
//! @brief  Generalized Radix Trie implementation
//! @detail Certain class of applications work very well with tries. Examples:
//!         1. Suffix Trees for {longest common} substring, regexp, etc.
//!         2. Longest Prefix Match for IP Routing.
//!         In generally any positive or negative substring match, such as
//!         longest, shortest, >= length n, etc.
//!        
//!         Complexity
//!         - Find/Modify(S): log(n) [where n is length of S] 
//!         - FindNext: log(n) time. 
//!         - 
//!         - Thread Safety: NOT thread safe i.e. NOT internally synchronized. 
//!           TODO: ConcurTrie: concurrent trie that is interally synchronized.
//!         
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_RADIX_TRIE_H_
#define _UTILS_DS_RADIX_TRIE_H_

// C++ Standard Headers
#include <array>            // std::array
#include <iomanip>          // std::setwidth
#include <memory>           // std::unique_ptr
#include <sstream>          // std::stringstream
#include <string>           // std::string
#include <utility>          // std::pair
// C Standard Headers
// Google Headers
// Local Headers

// The RadixTrie class is templatized to take in <Key,Value> as a
// template argument. 
// To avoid polluting the .h file with implementation details, 
// the method implementations have been moved to the .cc file, and
// specific instantiations are supported for the relevant <Key,Value> 
// types. Currently we support IPv4Prefix and StringPrefix.
// TODO: Add IPv6Prefix.

//! @addtogroup ds
//! @{

namespace asarcar { namespace utils { namespace ds {
//-----------------------------------------------------------------------------

// Forward Declarations
template <typename Key, typename Value>
class RadixTrie;

template <typename Key, typename Value>
std::ostream& operator << (std::ostream& os, 
                           const RadixTrie<Key,Value>& r);

template <typename Key, typename Value>
class RadixTrie {
 public:
  // Forward Declarations
  class Iterator;
  using KeyValue     = std::pair<Key,Value>;
  using KeyValuePtr  = KeyValue*;
  using KeyValueUPtr = std::unique_ptr<KeyValue>;
  using InsertRetType= std::pair<Iterator, bool>;

 private:
  // Forward Declarations
  class Node;
  using NodeUPtr = std::unique_ptr<Node>;
  using NodePtr      = Node*;
  class Node {
   public:
    // Tree is built using binary representation: 0 - left, 1 - right child
    static constexpr size_t NUM_CHILDREN= 2; 
    static constexpr size_t LEFT_CHILD  = 0; 
    static constexpr size_t RIGHT_CHILD = 1; 
    // Intentionally we pass input param by value.
    // Users are encouraged to pass rvalue for efficiency 
    // and support copy construction with move semantics.
    // Pass-by-value is optimal since we know for sure we are going to 
    // copy the argument. 
    // Beware as otherwise, we add a copy overhead if param is lvalue.
    Node(Key k, 
         KeyValueUPtr kv = nullptr, 
         NodePtr par_p = nullptr, 
         NodeUPtr lchild_p = nullptr,
         NodeUPtr rchild_p = nullptr); 
    Key           key; // key substring represented by this node
    KeyValueUPtr  keyvalue_p; // nullptr when node doesn't have associated value
    NodePtr       parent_p; // naked pointer
    std::array<NodeUPtr, NUM_CHILDREN> children_p;
  };
  
 public:
  RadixTrie() : root_p_{nullptr}, node_size_{0}, value_size_{0} {}
  ~RadixTrie() = default;

  inline size_t Size() const { return value_size_; }
  inline size_t NSize() const { return node_size_; }
  inline bool Empty() const { return (value_size_ == 0); }
  inline void Clear() {root_p_.reset(nullptr); value_size_ = 0; node_size_ = 0;}
  std::string to_string(bool dump_internal_nodes=false);

 private:
  std::string to_string(NodePtr node_p, int depth, bool dump_internal_nodes);

  Iterator Begin(NodePtr root_p) const;  
  inline Iterator End(NodePtr root_p) const { 
    return Iterator{this, root_p, nullptr};
  }
 public:
  inline Iterator Begin() const { return Begin(nullptr); }
  inline Iterator End() const { return End(nullptr); }
  Iterator Find(const Key& key) const; 
  Iterator LongestPrefixMatch(const Key& key) const;

  // Intentionally we pass input param by value.
  // Users are encouraged to pass rvalue for efficiency
  // and support copy construction with move semantics.
  // Pass-by-value is optimal since we know for sure we are going to 
  // copy the argument. 
  // Beware as otherwise, we add a copy overhead if param is lvalue.
  InsertRetType Insert(KeyValue kv);
  Iterator Erase(Iterator it);

  // Avoid: operator for first insert - wastes time default constructing
  // Value only to override it later.
  //
  // Intentionally we pass input param by value.
  // Users are encouraged to pass rvalue for efficiency
  // and support copy construction with move semantics.
  // Pass-by-value is optimal since we know for sure we are going to 
  // copy the argument. 
  // Beware as otherwise, we add a copy overhead if param is lvalue.
  inline Value& operator[] (Key key) {
    InsertRetType ret = Insert({std::move(key), Value{}});
    return ret.first->second;
  }

 private:
  NodeUPtr  root_p_;
  size_t    node_size_; // # trie node
  size_t    value_size_; // # trie nodes with value

  // Find the First, Next, and Prev Node of node_p in the subtree of root_p
  NodePtr GetFirst(const NodePtr root_p) const;
  NodePtr GetNext(const NodePtr root_p, const NodePtr node_p) const;
  NodePtr GetNextUp(const NodePtr root_p, const NodePtr node_p) const;

  // 1. Return Pointer to Node with the longest prefix match to key
  // 2. next_p: pointer to node with the first match violation to key
  // 3l lm_key_p: pointer to key as represented in longest prefix match node 
  NodePtr LongestPrefixMatchNode(const Key& key,
                                 const NodePtr root_p = nullptr,
                                 NodePtr* next_pp = nullptr,
                                 size_t* lm_key_len_p = nullptr,
                                 size_t* lp_key_len_p = nullptr) const;
  void SplitNode(NodePtr node_to_split_p, int len, NodePtr sibling_p);
  InsertRetType SetUpTreeBranch(KeyValue&& kv,
                                NodePtr    first_mm_node_p, 
                                int        lm_key_len, 
                                int        lp_key_len);

 public:
  class Iterator {
    friend class RadixTrie;
   public:
    inline bool operator ==(const Iterator& other) const { 
      return ((rt_p_==other.rt_p_) &&
              (node_p_==other.node_p_));
    }
    inline bool operator !=(const Iterator& other) const { 
      return !this->operator==(other);
    }
    inline Iterator& operator++() {
      node_p_ = rt_p_->GetNext(sub_root_p_, node_p_);
      return *this;
    }
    inline KeyValue     operator* () const { 
      DCHECK(node_p_ != nullptr);
      DCHECK(node_p_->keyvalue_p != nullptr);
      return *node_p_->keyvalue_p; 
    }
    inline KeyValuePtr  operator-> () const { 
      DCHECK(node_p_ != nullptr);
      DCHECK(node_p_->keyvalue_p != nullptr);
      return node_p_->keyvalue_p.get(); 
    }

   private:
    Iterator(const RadixTrie* rt_p, 
             const NodePtr root_p, 
             NodePtr node_p) : 
      rt_p_{rt_p}, 
      sub_root_p_{(root_p == nullptr)? rt_p->root_p_.get(): root_p}, 
      node_p_{node_p} {}

    // all nodes from root subtree to the next node we intend to visit
    const RadixTrie*     rt_p_;
    const NodePtr        sub_root_p_; 
    NodePtr              node_p_; 
  };
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace ds {

#endif // _UTILS_DS_RADIX_TRIE_H_
