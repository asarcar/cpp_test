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

#ifndef _UTILS_CONCUR_CONCUR_HASH_H_
#define _UTILS_CONCUR_CONCUR_HASH_H_

//! @file   concur_hash.h
//! @brief  Concurrent Hash Map
//! @detail Blocking thread safe simple hash implementation.
//!         TBD: Migrate to a non-blocking & wait-free 
//!         implementation based on: paper "High performance 
//!         dynamic lock-free hash tables and list-based sets" 
//!         - Maged Michael, SPAA 2002.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <memory>           // shared_ptr
#include <unordered_map>    // unordered_map
// C Standard Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/concur/spin_lock.h" // SpinLock
#include "utils/concur/lock_guard.h"// LockGuard

//! @addtogroup utils
//! @{

//! Namespace used for all concurrency utility routines
namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
template <typename Key, typename Value, typename LockType = SpinLock>
class ConcurHash {
 public:
  using ValuePtr = std::shared_ptr<Value>;
 private:
  using KVMap    = std::unordered_map<Key, ValuePtr>;
  using KVIter   = typename KVMap::iterator;
  using KVElem   = std::pair<Key, ValuePtr>;
 public:
  ConcurHash() : lck_{}, map_{} {}
  ~ConcurHash() = default;
  // Prevent bad usage: copy and assignment of Monitor
  ConcurHash(const ConcurHash&)             = delete;
  ConcurHash& operator =(const ConcurHash&) = delete;
  ConcurHash(ConcurHash&&)                  = delete;
  ConcurHash& operator =(ConcurHash&&)      = delete;

  // For all methods returning ValuePtr: 
  // While object pointed to by ValuePtr including memory is guaranteed 
  // to exist and user is free to subsequently modify the object, the hash 
  // entry itself may be deleted.

  // Insert fails i.e. Key exists => ValuePtr == nullptr
  inline ValuePtr Insert(Key&& key, Value&& value) {
    LockGuard<LockType>    lkg{lck_};
    auto res = map_.insert(KVElem{std::move(key), 
               std::make_shared<Value>(std::move(value))});
    if (!res.second)
      return nullptr;
    return res.first->second;
  }
  // Key doesn't exist => ValuePtr == null
  inline ValuePtr Find(const Key& key) {
    LockGuard<LockType>    lkg{lck_};
    auto res = map_.find(key);
    if (res == map_.end())
      return nullptr;
    return res->second;
  }
  // Key doesn't exist => bool == false
  inline bool Erase(const Key& key) {
    LockGuard<LockType>    lkg{lck_};
    return (map_.erase(key) > 0);
  }
  // Empty out all the key/value pairs in this map.
  inline void Clear(void) {
    LockGuard<LockType>    lkg{lck_};
    map_.clear();
  }
  inline size_t Size(void) {
    LockGuard<LockType>    lkg{lck_};
    return map_.size();
  }
  
 private:
  LockType                           lck_;
  KVMap                              map_;
};

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
#endif // _UTILS_CONCUR_CONCUR_HASH_H_
