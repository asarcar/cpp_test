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

//! @file     mem_block.h
//! @brief    RAII (resource acquisition is initialization) way of obtaining
//            memory. Memory block exists as long as object exists
//            Includes small buffer optimization: memory not allocated from 
//            separate heap is size <= 16 bytes
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _UTILS_BASIC_MEMBLOCK_H_
#define _UTILS_BASIC_MEMBLOCK_H_

// C++ Standard Headers
#include <iostream>
#include <functional>       // std::function
#include <memory>           // std::shared_ptr
// C Standard Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"

//! Generic namespace used for all utility routines developed
namespace asarcar {
//-----------------------------------------------------------------------------

//! @class    MemBlock
//! @brief    Class that captures the memory block
class MemBlock {
 public:
  using Ptr      = std::shared_ptr<MemBlock>;
  using PtrConst = std::shared_ptr<const MemBlock>;
  
  using  FreeFn_f = std::function<void(void*)>;
  static constexpr size_t MAX_MEMBLOCK_INLINE_SIZE{16};
  static const FreeFn_f NullFreeFn;


  explicit MemBlock(size_t mem_size=0, void *data_p=nullptr,
                    const FreeFn_f& freeFn = NullFreeFn);
  MemBlock(MemBlock &&);
  inline ~MemBlock() { Reset(0, nullptr); }
  // Prevent bad usage: copy and assignment of Memory Block
  MemBlock(const MemBlock&)             = delete;
  MemBlock& operator =(const MemBlock&) = delete;

  /**
   * Reset called in three flavors:
   * 1) Called with just new_size in which case MemBlock allocates/manages memory
   * 2) Called with new_size and new_data == nullptr (FreeFn must not be set)
   * 3) Called with new_size, new_data, and FreeFn set.
   * For 2) and 3)   
   *    a) If new_size > MAX_MEMBLOCK_INLINE_SIZE MemBlock allocates memory 
   *       otherwise memory is consumed from within object.
   *    b) If new_data != null_ptr, memory is handed over the MemBlock.
   *       In that case it is expected that FreeFn is also handed to MemBlock
   *       to allow it to free memory if so desired by the caller.
   */
  void Reset(size_t new_size, void *new_data=nullptr, 
             const FreeFn_f& freeFn = NullFreeFn);

  void *data(void);

  inline size_t size(void) { return size_; }

  static inline Ptr 
  Create(size_t mem_size = 0, void *data_p=nullptr, 
         const FreeFn_f& freeFn = NullFreeFn) { 
    return std::make_shared<MemBlock>(mem_size, data_p, freeFn);
  }
 private:
  size_t   size_;
  union Data {
    void                                          *p_;
    std::array<uint8_t, MAX_MEMBLOCK_INLINE_SIZE> buf_;
  } data_;
  FreeFn_f freeFn_;

  void AllocMem(size_t size, void *data_p, const FreeFn_f& freeFn);
}; // class MemBlock

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _UTILS_BASIC_MEMBLOCK_H_
