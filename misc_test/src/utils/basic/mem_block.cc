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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

//! @file   mem_block.cc
//! @brief  Implementation: Memory Block with Small Buffer Optimization
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <iostream>
#include <utility>    // std::swap
// Standard C Headers
// Google Headers
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/fassert.h"
#include "utils/basic/mem_block.h"

using namespace std;

namespace asarcar {
//-----------------------------------------------------------------------------

constexpr size_t  MemBlock::MAX_MEMBLOCK_INLINE_SIZE;
const MemBlock::FreeFn_f MemBlock::NullFreeFn = MemBlock::FreeFn_f();

MemBlock::MemBlock(size_t mem_size, void *data_p,
                   const FreeFn_f& freeFn) : 
size_{mem_size}, data_{data_p}, freeFn_{freeFn} {
  AllocMem(mem_size, data_p, freeFn);
}

MemBlock::MemBlock(MemBlock&& new_block) {
  swap(this->size_, new_block.size_);
  swap(this->data_, new_block.data_);
  swap(this->freeFn_, new_block.freeFn_);
}

void MemBlock::Reset(size_t new_size, void *new_data_p, 
                     const FreeFn_f& freeFn) {
  // Free memory assuming freeFn_ was set per application request or per MemBlock
  if ((size_ > MAX_MEMBLOCK_INLINE_SIZE) && (freeFn_ != nullptr))
    freeFn_(data_.p_);

  size_ = new_size; data_.p_ = new_data_p; freeFn_ = freeFn;
  AllocMem(new_size, new_data_p, freeFn);

  return;
}

void* MemBlock::data(void) {
  if (size_ == 0)
    return nullptr;

  if (size_ <= MAX_MEMBLOCK_INLINE_SIZE)
    return static_cast<void *>(data_.buf_.data());

  return data_.p_;
}

void MemBlock::AllocMem(size_t size, void *data_p, const FreeFn_f& freeFn) {
  // when size <= Threshold for SBO, buffer should be null
  FASSERT((size > MAX_MEMBLOCK_INLINE_SIZE) || (data_p == nullptr));
  // when data_p == nullptr && size > MAX_MEMBLOCK_INLINE_SIZE, freeFn cannot be Null
  FASSERT((data_p != nullptr) || (size <= MAX_MEMBLOCK_INLINE_SIZE) || (freeFn == nullptr));

  // No Memory Allocation: Small Buffer Optimization or when external buffer passed
  if ((size <= MAX_MEMBLOCK_INLINE_SIZE) || (data_p != nullptr))
    return;

  data_.p_ = malloc(size);
  freeFn_ = &free;
  return;
}


//-----------------------------------------------------------------------------
} // namespace asarcar
