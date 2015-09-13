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
    Node(Key&& k, 
         KeyValueUPtr&& kv = nullptr, 
         NodePtr par_p = nullptr, 
         NodeUPtr&& lchild_p = nullptr,
         NodeUPtr&& rchild_p = nullptr); 
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

  InsertRetType Insert(KeyValue&& kv);
  Iterator Erase(Iterator it);

  // Avoid this operator for first insert - wastes time default constructing
  // Value only to override it later.
  Value& operator[] (const Key& key);

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

template <typename Key, typename Value>
RadixTrie<Key,Value>::Node::Node(Key&& k, KeyValueUPtr&& kv, 
                                 NodePtr par_p,
                                 NodeUPtr&& lchild_p, NodeUPtr&& rchild_p) : 
    key{std::move(k)}, keyvalue_p{std::move(kv)}, 
  parent_p{par_p}, 
  children_p{std::move(lchild_p), std::move(rchild_p)} {
  if (children_p.at(Node::LEFT_CHILD) != nullptr)
    children_p.at(Node::LEFT_CHILD)->parent_p = this;
  if (children_p.at(Node::RIGHT_CHILD) != nullptr)
    children_p.at(Node::RIGHT_CHILD)->parent_p = this;
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::Iterator
RadixTrie<Key,Value>::Begin(NodePtr root_p) const { 
  root_p = (root_p == nullptr) ? root_p_.get() : root_p;
  return Iterator{this, root_p, GetFirst(root_p)}; 
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::Iterator
RadixTrie<Key,Value>::Find(const Key& key) const { 
  NodePtr lm_node_p = LongestPrefixMatchNode(key);
  if ((lm_node_p == nullptr) ||
      (lm_node_p->keyvalue_p == nullptr) || 
      (lm_node_p->keyvalue_p->first != key))
    return End();
  
  return Iterator{this, nullptr, lm_node_p};
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::Iterator
RadixTrie<Key,Value>::LongestPrefixMatch(const Key& key) const {
  // Traverse up from the longest match node to the first and nearest
  // ancestor that has value associated
  NodePtr lm_node_p = LongestPrefixMatchNode(key);  
  while ((lm_node_p != nullptr) && (lm_node_p->keyvalue_p == nullptr))
    lm_node_p = lm_node_p->parent_p;
  return Iterator{this, nullptr, lm_node_p};
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::Iterator
RadixTrie<Key,Value>::Erase(const Iterator it) {
  if (it == End())
    return it;
  NodePtr node_p = it.node_p_;

  DCHECK(node_p->keyvalue_p != nullptr);
  node_p->keyvalue_p.reset(nullptr); // frees <key,value> pair
  --value_size_;

  NodePtr parent_p = node_p->parent_p;
  NodePtr lchild_p = node_p->children_p.at(Node::LEFT_CHILD).get();
  NodePtr rchild_p = node_p->children_p.at(Node::RIGHT_CHILD).get();
  NodePtr child_p;

  NodePtr next_p   = nullptr;
  bool    next_chosen = false;
  size_t  idx = Node::LEFT_CHILD;

  // Cases:
  // a. Node is intermediate. 
  //    Both children exists we are done. Otherwise, if only one
  //    child exists: we will fall through to case (c).
  if ((lchild_p != nullptr) && (rchild_p != nullptr)) {
    next_p = GetNext(root_p_.get(), node_p);
    return Iterator{this, nullptr, next_p};
  }

  // b. Node is leaf. Discard leaf node. 
  //    If parent has no value fall to case (c) with Node updated as parent.
  if ((lchild_p == nullptr) && (rchild_p == nullptr)) {
    if (parent_p == nullptr) {
      // frees node_p and unreleased descendants of node_p
      root_p_.reset(nullptr); 
      --node_size_;
      return Iterator{this, nullptr, next_p};
    }

    child_p = parent_p->children_p.at(Node::LEFT_CHILD).get();
    idx = (child_p == node_p) ? Node::LEFT_CHILD : Node::RIGHT_CHILD;

    // frees node_p & unreleased descendants of node_p
    parent_p->children_p.at(idx).reset(nullptr); 
    --node_size_;

    node_p   = parent_p;
    parent_p = node_p->parent_p;
    lchild_p = node_p->children_p.at(Node::LEFT_CHILD).get();
    rchild_p = node_p->children_p.at(Node::RIGHT_CHILD).get();
    
    // Only case different from case C is getting Next when 
    // left child of parent is sibling of node whose value
    // was just deleted. The left branch nodes inorder traversal
    // order is prior to the currently visited node.
    // Thus next candidate is GetNextUp of left child.
    // For all other cases next is Next is GetNext(root_p, node_p)
    if (lchild_p != nullptr) {
      next_p = GetNextUp(root_p_.get(), lchild_p);
      next_chosen = true;
    }
  }

  // b fallthrough and c housekeeping
  DCHECK(node_p != nullptr);
  next_p = (next_chosen) ? next_p : GetNext(root_p_.get(), node_p);

  // b. fallthrough
  if (node_p->keyvalue_p != nullptr)
    return Iterator{this, nullptr, next_p};

  // c. Node intermediate with no value and only one child.
  //    Discard Node and "Promote" its single child to its place.
  idx = (rchild_p == nullptr) ? Node::LEFT_CHILD : Node::RIGHT_CHILD;
  // Release node_p's unique pointer "hold on" 
  // child and descendants of child
  child_p = node_p->children_p.at(idx).release();
  //    "Promoting" child implies that the prefix key of the child 
  //    includes the prefix key of the discarded node 
  child_p->key = node_p->key + child_p->key;

  if (parent_p == nullptr) {
    // frees root_p/node_p and unreleased descendants of root_p/node_p
    root_p_.reset(child_p); 
    --node_size_;
    child_p->parent_p = nullptr;
    return Iterator{this, nullptr, next_p};
  }

  // parent_p exists: Link the child to node's parent in the same branch path
  NodePtr parent_child_p = parent_p->children_p.at(Node::LEFT_CHILD).get();
  idx = (parent_child_p == node_p) ? Node::LEFT_CHILD : Node::RIGHT_CHILD;
  child_p->parent_p = parent_p;
  // frees node_p and unreleased descendants of node_p
  parent_p->children_p.at(idx).reset(child_p); 
  --node_size_;

  return Iterator{this, nullptr, next_p};
}

template <typename Key, typename Value>
Value& 
RadixTrie<Key,Value>::operator[] (const Key& key) {
  InsertRetType ret = Insert({key, Value{}});
  return ret.first->second;
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::NodePtr
RadixTrie<Key,Value>::LongestPrefixMatchNode(const Key& key,
                                             const NodePtr root_p,
                                             NodePtr* first_mm_node_pp,
                                             size_t* lm_key_len_p,
                                             size_t* lp_key_len_p) const {
  NodePtr  node_p   = (root_p == nullptr) ? root_p_.get() : root_p;
  NodePtr  parent_p = (node_p == nullptr) ? nullptr : node_p->parent_p;
  Key      k        = key;

  // first_mm_node_pp and lm_key_p points to local var if nullptr is passed
  // Cleaner code: no need to track null pointer before *pointer assignment
  NodePtr  first_mm_node_p;
  first_mm_node_pp = (first_mm_node_pp == nullptr) ? 
                     &first_mm_node_p : first_mm_node_pp;
  *first_mm_node_pp= nullptr; // default value 

  // lm_key_len_p and lp_key_len_p points to a local vars if nullptr is passed
  // Cleaner code: no need track nullptr before *pointer assignment
  size_t lm_key_len, lp_key_len;
  lm_key_len_p = (lm_key_len_p == nullptr) ? &lm_key_len : lm_key_len_p;
  lp_key_len_p = (lp_key_len_p == nullptr) ? &lp_key_len : lp_key_len_p;
  *lm_key_len_p= *lp_key_len_p = 0; // default value 

  while (node_p != nullptr) {
    int len_key    = k.size();
    int len_node   = node_p->key.size();
  
    Key pref       = k.prefix(node_p->key);
    int len_common = pref.size();
    *lp_key_len_p += len_common;

    // Node Key is not subsumed within key - longest match is the parent of node
    if (len_common < len_node) {
      *first_mm_node_pp = node_p;
      return parent_p;
    }

    // Assured that len_common = len_node => key of longest match 
    // node at least extends upto pref.
    *lm_key_len_p  += len_common;

    // Node Key subsumed within key: From here on: len_common == len_node
    // Key subsumed within NodeKey && vice-versa: LongestPrefixMatchNode Found!
    // exact match: 1st mismatch meaningless 
    // i.e. *first_mm_node_pp = nullptr default val
    if (len_common == len_key) 
      return node_p; 
    
    // Key bits beyond Node Key may be stored in the children of the root.
    // Continue exploration if possible

    int child_idx = k[len_common];
    DCHECK(child_idx < Node::NUM_CHILDREN);
    NodePtr child_p{node_p->children_p.at(child_idx).get()};

    // New key is (len_common, len_key] substring of previous key
    k = k.substr(len_common, (len_key - len_common));

    // New parent is node
    parent_p = node_p;    

    // New root is child
    node_p   = child_p;
  }

  // Traversed down tree till very end. Last parent is the longest match 
  // tree leaf: 1st mismatch meaningless 
  // i.e. *first_mm_node_pp = nullptr default val
  return parent_p;
}

// SplitNode is called because we are trying to insert a key where 
// prefix match is partial to the Node 
// After the split node operation the node returned will exactly 
// match the key until len bits (aka symbols)
template <typename Key, typename Value>
void RadixTrie<Key,Value>::SplitNode(NodePtr node_to_split_p, 
                                     int len,
                                     NodePtr sibling_p) {
  DCHECK(node_to_split_p != nullptr);
  int key_size = node_to_split_p->key.size();
  DCHECK(len < key_size);

  // Create the split child node. 
  // Insert a New Node between node_to_split and its children i.e. 
  // New Node inherits all fields of node_to_split: e.g. kv, children, ...
  NodePtr child_p =
      new Node{node_to_split_p->key.substr(len, (key_size - len)), 
               std::move(node_to_split_p->keyvalue_p),
               node_to_split_p, 
               std::move(node_to_split_p->children_p.at(Node::LEFT_CHILD)), 
               std::move(node_to_split_p->children_p.at(Node::RIGHT_CHILD))};
  node_size_++;

  // Link the new node as the split node's child
  size_t child_idx = node_to_split_p->key[len];
  DCHECK(node_to_split_p->children_p.at(child_idx) == nullptr);
  node_to_split_p->children_p.at(child_idx).reset(child_p);

  // Link the sibling as the split node's other child
  size_t sibling_idx = Node::RIGHT_CHILD - child_idx;
  DCHECK(node_to_split_p->children_p.at(sibling_idx) == nullptr);
  node_to_split_p->children_p.at(sibling_idx).reset(sibling_p);

  // Reset the length of the key of the current parent node appropriately
  node_to_split_p->key.resize(len);
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::InsertRetType
RadixTrie<Key,Value>::Insert(KeyValue&& kv) {
  size_t  key_len = kv.first.size();
  NodePtr root_p{root_p_.get()};
  NodePtr first_mm_node_p{nullptr};
  size_t  lm_key_len{0}; 
  size_t  lp_key_len{0};
  NodePtr lm_node_p = LongestPrefixMatchNode(kv.first, root_p, 
                                             &first_mm_node_p,
                                             &lm_key_len,
                                             &lp_key_len); 
  DCHECK(lm_key_len <= key_len);
  DCHECK(lm_key_len <= lp_key_len);
  DCHECK(lp_key_len <= key_len);

  // Cases
  // a. Split Intermediate Node
  //      First Mismatch Node Found
  // a.1. Longest Match Node ! Found or Found with shorter key
  //      Note: Longest Match Node Found with equal key not
  //      possible as First Mismatch Node is returned as nullptr in that case
  //
  // First Mismatch Node ! Found cases from here on.
  // b. New Root Node: 
  //                Longest Match Node ! found.
  // 
  // Longest Match Node Found cases from here on.
  // c. New Leaf Node: 
  //                Longest Match node found with shorter key.
  // d. Exact match:   
  //                Longest Match Node found with exact key.

  // a.
  if (first_mm_node_p != nullptr) {
    DCHECK(lm_key_len < lp_key_len);
    // a.1 or a.2.
    NodePtr sibling_p = 
      new Node{kv.first.substr(lp_key_len, (key_len - lp_key_len)), 
               KeyValueUPtr{new KeyValue{std::move(kv)}}, first_mm_node_p};
    ++node_size_; ++value_size_;

    SplitNode(first_mm_node_p, lp_key_len - lm_key_len, sibling_p);
    DCHECK(first_mm_node_p->keyvalue_p == nullptr);
    DCHECK(sibling_p->keyvalue_p != nullptr);
    return InsertRetType{Iterator{this, root_p, sibling_p}, true};
  }
  
  // First mismatch node ! Found cases from here on
  // b. New Root Node
  if (lm_node_p == nullptr) {
    DCHECK(lm_key_len == 0); 
    DCHECK(root_p == nullptr);
    root_p = new Node{Key{kv.first}, KeyValueUPtr{new KeyValue{std::move(kv)}}};
    root_p_.reset(root_p);
    ++node_size_; ++value_size_;
    return InsertRetType{Iterator{this, root_p, root_p}, true};
  }

  // Longest Match Node Found cases from here on
  // c. Exact Match
  if (lm_key_len == key_len) {
    DCHECK(lp_key_len == key_len);
    bool new_insert = false;
    if (lm_node_p->keyvalue_p == nullptr) {
      lm_node_p->keyvalue_p.reset(new KeyValue{std::move(kv)});
      ++value_size_;
      new_insert = true;
    }
    return InsertRetType{Iterator{this, root_p, lm_node_p}, new_insert};
  }

  // d. New Leaf Node. 
  DCHECK(lm_key_len == lp_key_len);
  DCHECK(lm_key_len < key_len && lp_key_len < key_len);
  size_t child_idx = kv.first[lm_key_len];
  DCHECK(lm_node_p->children_p.at(child_idx) == nullptr);
  NodePtr child_p = 
      new Node{kv.first.substr(lm_key_len, (key_len - lm_key_len)), 
               KeyValueUPtr{new KeyValue{std::move(kv)}}, lm_node_p};
  ++node_size_; ++value_size_;
  lm_node_p->children_p.at(child_idx).reset(child_p);
  return InsertRetType{Iterator{this, root_p, child_p}, true};
}


template <typename Key, typename Value>
typename RadixTrie<Key,Value>::NodePtr 
RadixTrie<Key,Value>::GetFirst(const NodePtr root_p) const {  
  NodePtr node_p = root_p; 
  NodePtr child_p;

  while (node_p != nullptr) {
    if (node_p->keyvalue_p != nullptr) {
      return node_p;
    }

    child_p = node_p->children_p.at(Node::LEFT_CHILD).get();
    child_p = (child_p == nullptr) ? 
              node_p->children_p.at(Node::RIGHT_CHILD).get() : child_p;
    
    node_p = child_p;
  }
  
  return nullptr;
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::NodePtr 
RadixTrie<Key,Value>::GetNext(const NodePtr root_p, 
                              const NodePtr node_p) const {
  DCHECK(root_p != nullptr);
  DCHECK(node_p != nullptr);

  // Subtree under node_p left (1st) or right (2nd) exists?
  // If so the first candidate in that subtree is Next candidate
  NodePtr child_p = node_p->children_p.at(Node::LEFT_CHILD).get();
  child_p = (child_p == nullptr) ? 
            node_p->children_p.at(Node::RIGHT_CHILD).get() : child_p;
  NodePtr next_p = GetFirst(child_p);
  if (next_p != nullptr)
    return next_p;
  
  next_p = GetNextUp(root_p, node_p);
  return next_p;
}

template <typename Key, typename Value>
typename RadixTrie<Key,Value>::NodePtr 
RadixTrie<Key,Value>::GetNextUp(const NodePtr root_p, 
                                const NodePtr node_p) const {
  DCHECK(root_p != nullptr);
  DCHECK(node_p != nullptr);

  // Traverse up node_p until terminating condition i.e. traverse above root_p
  // Otherwise
  //    1. keep going up until we 
  //         reach a parent with whom we connect to the left branch side
  //         if first node exists to the right branch we have next candidate
  //    2. Otherwise step 1.
  NodePtr child_p = node_p;
  NodePtr par_p   = node_p->parent_p;
  while ((par_p != nullptr) && 
         (par_p != root_p->parent_p) && 
         (child_p != nullptr)) {
    NodePtr rchild_p = par_p->children_p.at(Node::RIGHT_CHILD).get();

    // if child_p was left child of parent and rchild of parent exists
    if ((rchild_p != child_p) && (rchild_p != nullptr)) {
      NodePtr next_p = GetFirst(rchild_p);
      DCHECK(next_p != nullptr);
      return next_p;
    }

    // Either we traversed up the right branch or right branch did not exist.
    // Keep traversing up in search of the next candidate
    child_p = par_p;
    par_p  = child_p->parent_p;
  }

  return nullptr;
}

template <typename Key, typename Value>
std::string 
RadixTrie<Key,Value>::to_string(bool dump_internal_nodes) {
  std::ostringstream oss;  

  oss << "#nodes " << node_size_ << ": #values " << value_size_ << std::endl;
  oss << "---------------------------" << std::endl;
  oss << this->to_string(root_p_.get(), 0, dump_internal_nodes);
  oss << "===========================" << std::endl;

  return oss.str();
}

template <typename Key, typename Value>
std::string 
RadixTrie<Key,Value>::to_string(NodePtr node_p, int depth, bool dump_internal_nodes) {
  if (node_p == nullptr)
    return std::string("");
  
  NodePtr lchild_p = node_p->children_p.at(Node::LEFT_CHILD).get();
  NodePtr rchild_p = node_p->children_p.at(Node::RIGHT_CHILD).get();
  
  std::ostringstream oss;
  if (dump_internal_nodes)
    oss << std::setw(4*(depth+1)) << std::setfill(' ')
        << "[" << depth << "] "
        << std::hex << node_p << " (" << node_p->parent_p << ")"
        << " {" << lchild_p << "," << rchild_p << "} "
        << "prefix " <<  node_p->key;
  if (node_p->keyvalue_p != nullptr)
    oss << " <" << node_p->keyvalue_p->first << ","
        << node_p->keyvalue_p->second  << ">";
  if (dump_internal_nodes || (node_p->keyvalue_p != nullptr))
    oss << std::endl;

  std::string lchild_str = to_string(lchild_p, depth+1, dump_internal_nodes);
  std::string rchild_str = to_string(rchild_p, depth+1, dump_internal_nodes);

  return oss.str() + lchild_str + rchild_str;
}

template <typename Key, typename Value>
std::ostream& operator << (std::ostream& os, 
                           const RadixTrie<Key,Value>& rt) {

  os << "#nodes " << rt.NSize() << ": #values " << rt.Size() << std::endl;
  os << "---------------------------" << std::endl;

  typename RadixTrie<Key,Value>::Iterator it = rt.Begin();
  typename RadixTrie<Key,Value>::Iterator itend = rt.End();

  for(; it != itend; ++it)
    os << "<" << it->first << "," << it->second << ">" << std::endl;
  
  os << "===========================" << std::endl;

  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace ds {

#endif // _UTILS_DS_RADIX_TRIE_H_
