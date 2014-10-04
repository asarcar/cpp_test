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

// Standard C++ Headers
#include <iostream>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/basic/mem_block.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class MemBlockTester {
 public:
  void Run(void) {
    // Default construction and destruction
    {
      MemBlock::Ptr buf = make_shared<MemBlock>();
      CHECK(buf->data() == nullptr);
      CHECK_EQ(buf->size(), 0);
    }
    // Size Construction
    {
      MemBlock::Ptr buf = make_shared<MemBlock>(MemBlock::MAX_MEMBLOCK_INLINE_SIZE);
      CHECK(buf->data() != nullptr);
      CHECK_EQ(buf->size(), MemBlock::MAX_MEMBLOCK_INLINE_SIZE);
    }
    
    // Reset with Size
    {
      MemBlock::Ptr buf = make_shared<MemBlock>(32);
      void *data = buf->data();
      buf->Reset(MemBlock::MAX_MEMBLOCK_INLINE_SIZE);
      CHECK_EQ(buf->size(), MemBlock::MAX_MEMBLOCK_INLINE_SIZE);
      CHECK_NE(buf->data(), data);
    }

    // Reset With Data/Size: 
    // 1. Validate MemBlock doesn't attempt to free memory when destructor called
    char s[] = "MemBlock assumes memory when size>threshold";
    {
      MemBlock::Ptr buf = make_shared<MemBlock>(8);
      buf->Reset(strlen(s), s);
      CHECK_EQ(memcmp(buf->data(), s, buf->size()), 0);
    }

    // Reset With Data/Size: 
    // 2. Validate MemBlock frees memory when we pass freeFn and Reset called again
    // 3. Validate MemBlock frees memory when we pass freeFn and MemBlock is destroyed
    int free_call = 0;
    {
      size_t siz = MemBlock::MAX_MEMBLOCK_INLINE_SIZE*2;
      void  *mem = malloc(siz);
      auto freeFn = [&free_call](void *p){free(p); ++free_call;};
      MemBlock::Ptr buf = make_shared<MemBlock>(siz, mem, freeFn);
      size_t siz2 = MemBlock::MAX_MEMBLOCK_INLINE_SIZE*4;
      void  *mem2 = malloc(siz2);
      buf->Reset(siz2, mem2, freeFn);
      CHECK_EQ(free_call, 1);
    }
    CHECK_EQ(free_call, 2);
  }
};

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  MemBlockTester mbt;
  mbt.Run();

  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
