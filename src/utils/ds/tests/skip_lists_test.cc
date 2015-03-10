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
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/ds/skip_lists.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class SkipListTester {
 public:
  void BitComputeTest(void);
  void NoNodeTest(void);
  void OneNodeTest(void);
  void FullTest(void);
 private:
};

void SkipListTester::BitComputeTest(void) {
  constexpr uint32_t N = 0x01011001;
  constexpr uint32_t O = NumOnes(N);
  constexpr uint32_t M = MsbPos(N);
  constexpr uint32_t L = LogBaseTwoCeiling(N);

  LOG(INFO) << "N=" << N << ": O="<< O << ": M=" << M << ": L=" << L;
  CHECK_EQ(O, 4);
  CHECK_EQ(M, 25);
  CHECK_EQ(L, 26);
  CHECK_EQ(LogBaseTwoCeiling(0x01000000), 25);
}

void SkipListTester::NoNodeTest(void) {
  using SkipListType=SkipList<int,1>;
  SkipListType s;
  CHECK_EQ(s.MaxLevel(), 1);
  
  SkipListType::NodePtrC h = s.Head();
  CHECK_EQ(h->Size(), 1);

  for(uint32_t i=0; i<h->Size(); ++i) {
    CHECK(h->Next(i) == nullptr);
  }
  return;
}

void SkipListTester::OneNodeTest(void) {
  return;
}

void SkipListTester::FullTest(void) {
  return;
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  SkipListTester st;
  st.BitComputeTest();
  st.NoNodeTest();
  st.OneNodeTest();
  st.FullTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
