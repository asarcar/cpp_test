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

//! @file   treap.h
//! @brief  Tree (Binary Sorted) & Heap: each node has an added randomized priority  
//! @detail Red Black Trees create balanced trees that bound:
//!         - Find/Modify:  log(n). 
//!         Treaps provide: 
//!         - Find/Modify/Next: log(n) statistically. 
//!         - Allows to optimize access to frequently accessed data elements.
//!           Prioritize nodes with keys/values that are accessed multiple time. 
//!           Ensures that highly accessible nodes are accessed in fewer operations.
//!         - Memory: One additioanl word per node for priority. 
//!         - Thread Safety: NOT thread safe. For concurrent R/RW access use
//!           synchronization. 
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_TREAP_H_
#define _UTILS_DS_TREAP_H_

// C++ Standard Headers
#include <iomanip>          // std::setw
#include <iostream>         // std::ostream
#include <functional>       // std::function
#include <random>           // std::distribution, random engine, ...
// C Standard Headers
// Google Headers
#include <glog/logging.h>
// Local Headers
#include "utils/basic/fassert.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils {
//-----------------------------------------------------------------------------

// Forward Declarations
template <typename K, typename V>
class Treap;

template <typename K, typename V>
class TreapCIter;

template <typename K, typename V>
std::ostream& operator << (std::ostream& os, const Treap<K,V>& t);

// Assumed following "Concepts" defined.
// 1. K ordering: Ki < Kj, Ki > Kj, and Ki == Kj
// 2. K/V Initialization and Creation: Copy and Assign rval ctors 
// 3. ostream operators defined: os << K. os << V
template <typename K, typename V>
class Treap {
 private:
  struct Node;
 public:
  using NodePtr  = Node *;
  using TreapPtr = const Treap<K,V>* const;
  using Priority = uint32_t;
  using AccFn    = const std::function<uint32_t(NodePtr, uint32_t)>&;

  Treap() : 
      _num_nodes{0}, _root{nullptr}, _seed{std::random_device{}()},
      _random{std::bind(std::uniform_int_distribution<uint32_t>
                      {1, std::numeric_limits<Priority>::max()}, 
                      std::default_random_engine{_seed})} {} 
  ~Treap() {
  }

  // Iterator
  friend class TreapCIter<K, V>;
  using  ItC   = TreapCIter<K, V>;
  // type returned for Modify operations Emplace/Insert/Remove
  using  ModPr = std::pair<ItC,bool>; 

  inline ItC begin(void) {
    return ItC{this};
  }
  inline ItC last() {
    NodePtr p = getLast(_root);
    return ItC{this, _root, p};
  }
  inline ItC end(void) {
    return ItC{this, _root, nullptr};
  }
  inline ItC begin(const K& key) {
    NodePtr root = get(_root, key);
    return ItC{this, root};
  }
  inline ItC last(const K& key) {
    NodePtr root = get(_root, key);
    NodePtr p = getLast(root);
    return ItC{this, root, p};
  }
  inline ItC end(const K& key) {
    NodePtr root = get(_root, key);
    return ItC{this, root, nullptr};
  }

  // Operations: Find/Insert/Remove...
  // Iterator == end() if element not found
  inline ItC Find(const K& key) const {
    NodePtr p = get(_root, key);
    return ItC(this, _root, p);
  }

  // Constructs element in place and Inserts to Treap if Value does not exist.
  // Returns <Iterator,false> if element exists (insert failed) else <iterator, true>
  inline ModPr Emplace(K &&key, V&& val) {
    NodePtr p = nullptr;
    bool exists = add(&_root, std::move(key), std::move(val), &p);
    return ModPr{ItC{this, _root, p}, exists};
  }

  inline ModPr Insert(K &key, V& val) { 
    return Emplace(K{key}, V{val}); 
  }

  // Remove: removes element
  // returns first element following the element removed bool (TRUE) if
  // remove candidate indeed existed
  inline ModPr Remove(const K& key) {
    // suboptimal implementation: many cases the deleted key and next element
    // may lie in the same subtree in which can one could avoid one log(n) 
    // traversal. In this case we are traversing log(n) twice.
    return ModPr{ItC{this, _root, getNext(_root, key)}, Delete(key)};
  }

  // Delete: removes element & returns bool (TRUE if element was removed when it exists)
  inline bool Delete(const K& key) {
    return del(&_root, key);
  }

  // InOrder: Inorder Traverses the tree embedded in treap
  // Executes function on every node and returns the accumulated result
  inline uint32_t InOrder(AccFn fn) const {
    return inorder(fn, _root, 0);
  }

  // Size: returns number of nodes (K,V)s in the treap
  inline size_t Size(void) const {
    return _num_nodes;
  }

  // For repeatable & predictable node level generation, we 
  // generate the same seeds for random number when testing 
  // such that same random number is generated every time
  void SetPredictablePriority(void) {
    _seed = kFixedCostSeedForRandomEngine;
    _random = std::bind(std::uniform_int_distribution<uint32_t>
                        {1, std::numeric_limits<Priority>::max()}, 
                        std::default_random_engine{_seed}); 
  }

  // helper function to allow chained cout cmds: example
  // cout << "Treap: " << endl << t << endl << "---------" << endl;
  friend std::ostream& operator << <>(std::ostream& os, const Treap<K,V>& t);
  
 private:
  struct Node {
    Node(K &&key, V &&value): k(std::move(key)), v(std::move(value)){}
    const K   k; // immutable after creation
    V         v; // mutable
    Priority  pri; // mutable
    NodePtr   l; // left
    NodePtr   r; // right
  };

  uint32_t                      _num_nodes;
  NodePtr                       _root;
  Priority                      _seed;
  std::function<Priority(void)> _random;
                         
  //! Fixed seed generates predictable MC runs when running test SW or debugging
  const static uint32_t kFixedCostSeedForRandomEngine = 13607; 

  inline NodePtr newNode(K&& key, V&& val) {
    NodePtr np = new Node(std::move(key), std::move(val));
    np->l   = np->r = nullptr;
    np->pri =_random();
    ++_num_nodes;
    return np;
  }

  inline void deleteNode(NodePtr np) {
    --_num_nodes;
    // reset fields: allows one to identify stale pointers
    np->l   = np->r = nullptr;
    np->pri = 0;
    delete np;
  }

  //! @fn            add
  //! @param[in|out] ptr to root of subtree
  //! @param[in]     key (rvalue reference)
  //! @param[in]     value (rvalue reference)
  //! @param[out]    result node added
  //! @returns       boolean set to true if node is added (doesn't exist before)
  bool add(NodePtr *root_p, K &&key, V &&val, NodePtr *res_p) {
    FASSERT(root_p != nullptr);
    NodePtr root = *root_p;
    if (root == nullptr) {
      *root_p = *res_p = newNode(std::move(key), std::move(val));
      DLOG(INFO) << "Node (K,V)=(" << key << "," << val 
                 << "): P=" << (*res_p)->pri << " created";
      return true;
    }
    
    // key already exists
    if (key == root->k) {
      *res_p = root;
      return false;
    }
    
    // traverse down the point where we should add the node
    if (key < root->k) {
      bool added = add(&root->l, std::move(key), std::move(val), res_p);
      // "Bubble" up the node to the appropriate position in the heap
      if (root->l->pri <= root->pri)
        return added;
      DLOG(INFO) << "Node (K,V)=(" << root->k << "," << root->v 
                 << "): P=" << root->pri 
                 << ": Right Rotate as LChild (K,V)=(" 
                 << root->l->k << "," << root->l->v 
                 << "): P=" << root->l->pri;
      // Heap: invalid state: left child has higher priority than parent
      // Right Rotate: Left Child becomes the new root
      rotateRight(root_p);
      return added;
    }
    
    bool added = add(&root->r, std::move(key), std::move(val), res_p);
    // "Bubble" up the node to the appropriate position in the heap
    if (root->r->pri <= root->pri)
      return added;
    DLOG(INFO) << "Node (K,V)=(" << root->k << "," << root->v 
               << "): P=" << root->pri 
               << ": Left Rotate as RChild (K,V)=(" 
               << root->r->k << "," << root->r->v 
               << "): P=" << root->r->pri;
    // Heap: invalid state: right child has higher priority than parent
    // Left Rotate: Right Child becomes the new root
    rotateLeft(root_p);
    return added;
  }

  //! @fn            del
  //! @param[in|out] ptr to root of subtree
  //! @param[in]     key (rvalue reference)

  //! @param[out]    result node added
  //! @returns       boolean set to true if node is deleted (doesn't exist before)
  bool del(NodePtr *root_p, const K& key) {
    FASSERT(root_p != nullptr);
    NodePtr root = *root_p;
    if (root == nullptr)
      return false;

    if (key < root->k)
      return del(&root->l, key);

    if (key > root->k)
      return del(&root->r, key);

    // Key found: delete the node, promote children, and maintain heap sanity
    // Node is leaf node: nothing to promote, heap sanity maintained
    if (root->l == nullptr && root->r == nullptr) {
      DLOG(INFO) << "Node (K,V)=(" << root->k << "," << root->v 
                 << "): P=" << root->pri << ": deleted";
      deleteNode(root);
      *root_p = nullptr;
      return true;
    }
    
    Priority lPri = (root->l == nullptr) ? 0 : root->l->pri;
    Priority rPri = (root->r == nullptr) ? 0 : root->r->pri;

    if (lPri >= rPri) {
      DLOG(INFO) << "Node (K,V)=(" << root->k << "," << root->v 
                 << "): P=" << root->pri 
                 << ": Right Rotate as lPri>=rPri (" 
                 << lPri <<"," << rPri << ")";
      // Promote left child to maintain heap
      rotateRight(root_p);
      return del(&(*root_p)->r, key);
    }

    // Promote right child to maintain heap
    DLOG(INFO) << "Node (K,V)=(" << root->k << "," << root->v 
               << "): P=" << root->pri 
               << ": Left Rotate as lPri<rPri (" 
               << lPri <<"," << rPri << ")";
    rotateLeft(root_p);
    return del(&(*root_p)->l, key);
  }

  //! @fn         inorder traverses nodes of treap
  //! @param[in]  function executed on every node 
  //! @param[in]  root of subtree
  //! @returns    accumulated result
  inline uint32_t 
  inorder(AccFn fn, NodePtr root, uint32_t level) const {
    if (root == nullptr)
      return 0;

    uint32_t acc = inorder(fn, root->l, level+1);
    acc += fn(root, level);
    acc += inorder(fn, root->r, level+1);

    return acc;
  }

  //! @fn         get
  //! @param[in]  root of subtree
  //! @param[in]  key (const lvalue reference)
  //! @returns    ptr to node if it exists
  NodePtr get(NodePtr root, const K& key) const {
    if (root == nullptr)
      return nullptr;

    if (key == root->k)
      return root;

    if (key < root->k)
      return get(root->l, key);

    return get(root->r, key);
  }

  NodePtr getFirst(NodePtr root) const {
    if (root == nullptr)
      return nullptr;
    
    // first is first node in left subtree 
    if (root->l != nullptr)
      return getFirst(root->l);

    // first is current root
    return root;
  }

  NodePtr getLast(NodePtr root) const {
    if (root == nullptr)
      return nullptr;
    
    // last is last node in right subtree 
    if (root->r != nullptr)
      return getLast(root->r);

    // last is current root
    return root;
  }

  NodePtr getNext(NodePtr root, const K& key) const {
    if (root == nullptr)
      return nullptr;

    // key >= node-key implies next has to be in the right subtree
    if (key >= root->k) 
      return getNext(root->r, key);
    
    // root->k > key implies next could be in left subtree or if not then root 
    NodePtr p = getNext(root->l, key);
    if (p != nullptr)
        return p;
    return root;
  }

  NodePtr getPrev(NodePtr root, const K& key) const {
    if (root == nullptr)
      return nullptr;

    // key <= root->k implies previous has to be in the left subtree
    if (key <= root->k) 
      return getPrev(root->l, key);
    
    // key > root->k implies prev could be in right subtree or if not then root 
    NodePtr p = getPrev(root->r, key);
    if (p != nullptr)
        return p;

    return root;
  }

  // Left Rotation: return new root
  void rotateLeft(NodePtr* root_p) {
    FASSERT(root_p != nullptr); 
    NodePtr root = *root_p;
    FASSERT(root != nullptr);
    NodePtr right = root->r;
    FASSERT(right != nullptr);
    root->r = right->l;
    right->l = root;
    *root_p = right;
    return;
  }

  // Right Rotation: return new root
  void rotateRight(NodePtr* root_p) {
    FASSERT(root_p != nullptr); 
    NodePtr root = *root_p;
    FASSERT(root != nullptr);
    NodePtr left = root->l;
    FASSERT(left != nullptr);
    root->l = left->r;
    left->r = root;
    *root_p = left;
    return;
  }
};

// Random Access Iterator for Treaps. TODO: Should have READ ONLY access 
// to key (i.e. key attribute of T).
template <typename K, typename V>
class TreapCIter {
 public:
  using Tr          = Treap<K, V>;
  using TreapPtr    = typename Tr::TreapPtr;
  using NodePtr     = typename Tr::NodePtr;
  using ItC         = typename Tr::ItC;
  using KVPtrPair   = std::pair<const K*, V*>;
  // TODO: sanity check root is part of trp, np is part of root of nullptr
  TreapCIter(TreapPtr trp, NodePtr  root, NodePtr  np):
      _trp{trp}, _root{root}, _cur{np} { getKVP(); }
  TreapCIter(TreapPtr trp, NodePtr  root): 
      TreapCIter(trp, root, trp->getFirst(root)) {}
  TreapCIter(TreapPtr trp): 
      TreapCIter(trp, trp->_root, trp->getFirst(trp->_root)) {}
  TreapCIter& operator=(const TreapCIter& o) {
    // Leave _trp alone, copy other params
    _root = o._root; _cur = o._cur; _kvp = o._kvp; 
    return *this;
  }
  inline const KVPtrPair& operator*() {
    FASSERT(_cur != nullptr);
    return _kvp;
  }
  inline ItC& operator++() {
    FASSERT(_cur != nullptr);
    _cur = _trp->getNext(_root, _cur->k);
    getKVP();
    return *this;
  }
  inline ItC& operator--() {
    FASSERT(_cur != nullptr);
    _cur = _trp->getPrev(_root, _cur->k);
    getKVP();
    return *this;
  }
  inline bool operator==(const ItC &other) {
    return (this->_trp == other._trp && 
            this->_root == other._root && 
            this->_cur == other._cur);
  }
  inline bool operator!=(const ItC &other) {
    return !this->operator==(other);
  }
  inline NodePtr operator->(void) {
    return this->_cur;
  }
 private:
  TreapPtr    _trp;
  NodePtr     _root; // Subtree used to iterate
  NodePtr     _cur;
  KVPtrPair   _kvp;
  inline void getKVP(void) {
    _kvp.first = (_cur != nullptr) ? &_cur->k : nullptr;
    _kvp.second = (_cur != nullptr) ? &_cur->v : nullptr;
  }
};

// Treap Output
// helper function to allow chained cout cmds: example
// cout << "Treap: " << endl << t << endl << "---------" << endl;
template <typename K, typename V>
std::ostream& operator << (std::ostream& os, const Treap<K,V>& t) {
  using NodePtr = typename Treap<K,V>::NodePtr;

  os << std::endl << "#************************#" << std::endl;
  os << "# TREAP:                 #" << std::endl;
  os << "#------------------------#" << std::endl;
  os << "# Size=" << std::setfill(' ') << std::setw(10) 
     << std::setfill(' ') << std::left << t.Size() 
     << "--------#" << std::endl;
  os << "##########################" << std::endl;
  t.InOrder([&os] (NodePtr np, uint32_t level) {
      os << std::setfill(' ') << std::setw(level*2) 
         << std::right << "<" << level << ">:" 
         << "(" << np->k << "," << np->v << ")[" << np->pri <<"] " << std::endl;
      return 1;
    });
  os << "#************************#" << std::endl;
  
  return os;
}

//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace utils {

#endif // _UTILS_DS_TREAP_H_
