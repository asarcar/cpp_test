// Copyright 2016 asarcar Inc.
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

//! @file   elist.h
//! @brief  Efficient list(map) implementation for small number of elements
//! @detail list or unordered/ordered is a good data structure 
//!         for specific operations and workloads. 
//!         However, due to the memory hierarchy of modern processors, 
//!         these data structures are terrible. 
//!         Cache misses occur in simple operations.
//!         Thus, the inefficiency of vector for many operations
//!         are more than compensated by cache locality aspect.
//!         
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_DS_ELIST_H_
#define _UTILS_DS_ELIST_H_

// C++ Standard Headers
#include <initializer_list> // std::initializer_list
#include <iostream>         // std::ostream
#include <vector>           // std::array
#include <utility>          // std::pair
// C Standard Headers
// Google Headers
// Local Headers

// Elist class is templatized to take <Node> as a template argument. 

//! @addtogroup ds
//! @{

namespace asarcar { namespace utils { namespace ds {
//-----------------------------------------------------------------------------

// Forward Declarations
template <typename Node>
class Elist;
template <typename Node>
std::ostream& operator << (std::ostream& os, const Elist<Node>& l);

template <typename Node>
class Elist {
 public:
  Elist()                          = default;
  Elist(std::initializer_list<Node> init) : v_(init) {}
  ~Elist()                         = default;
  Elist(const Elist& o)            = delete;
  Elist& operator=(const Elist& o) = delete;
  Elist(Elist&& o)                 = delete; 
  Elist& operator=(Elist&& o)      = delete;
  
  // Iterator
  using iterator = typename std::vector<Node>::iterator;
  using const_iterator = typename std::vector<Node>::const_iterator;
  inline iterator begin(void) {return v_.begin();}
  inline const_iterator begin(void) const {return v_.cbegin();}
  inline iterator end(void) {return v_.end();}
  inline const_iterator end(void) const {return v_.cend();}
  template <typename Predicate>
  inline iterator find(Predicate pred) {
    iterator it;
    for (it=begin(); it!=end() && !pred(*it); ++it);
    return it;
  }

  // Access Operations
  inline Node& front(void) {return v_.front();}
  inline const Node& front(void) const {return v_.front();}
  inline Node& back(void) {return v_.back();}
  inline const Node& back(void) const {return v_.back();}

  // Modify Operations
  template <typename... Args > 
  iterator emplace(iterator pos, Args&&... args) {
    return v_.emplace(pos, std::forward<Args>(args)...);
  }
  template <typename... Args>
  inline void emplace_back(Args&&... args) {
    v_.emplace_back(std::forward<Args>(args)...);
  }
  inline void pop_back(void) {v_.pop_back();}

  iterator insert(iterator pos, Node&& value) {
    return v_.insert(pos, std::forward<Node>(value));
  }
  inline iterator erase(iterator pos) {
    return v_.erase(pos);
  }

  // Visibility Operations
  inline size_t size(void) const {return v_.size();}
  friend std::ostream& 
  operator << <>(std::ostream& os, const Elist<Node>& l);
 private:
  std::vector<Node> v_;
};

template <typename Node>
std::ostream& operator << (std::ostream& os, const Elist<Node>& l) {
  os << "Elist:";
  for(const Node& n: l.v_) {
    os << " " << n;
  }
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace ds {

#endif // _UTILS_DS_ELIST_H_
