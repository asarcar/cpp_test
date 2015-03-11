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
  constexpr uint32_t O = Ones(N);
  constexpr uint32_t M = MsbOnePos(N);

  LOG(INFO) << "N=" << hex << N << dec << ": O="<< O << ": M=" << M;
  CHECK_EQ(O, 4);
  CHECK_EQ(M, 25);
  CHECK_EQ(MsbOnePos(0x01000000), 25);

  constexpr uint32_t N1 = 0x00C0CFDB;
  constexpr uint32_t T1 = TrailingOnes(N1);
  constexpr uint32_t N2 = 0x00C0CFDA;
  constexpr uint32_t T2 = TrailingOnes(N2);

  LOG(INFO) << "N1=" << hex << N1 << dec <<": T1="<< T1 
            << ": N2=" << hex << N2 << dec << ": T2=" << T2;
  CHECK_EQ(T1, 2);
  CHECK_EQ(T2, 0);
} 

void SkipListTester::NoNodeTest(void) {
  using SkipListType=SkipList<int,1>;
  SkipListType s;
  CHECK_EQ(s.MaxLevel(), 1);
  
  SkipListType::NodePtr h = s.Head();
  CHECK_EQ(h->Size(), 1);

  for(uint32_t i=0; i<h->Size(); ++i) {
    CHECK(h->Next(i) == nullptr);
  }
  return;
}

void SkipListTester::OneNodeTest(void) {
  constexpr uint32_t N = 3;
  using SkipListType=SkipList<int,N>;
  SkipListType s;
  // Validate MaxLevel = Ceiling(log2(N))
  CHECK_EQ(s.MaxLevel(), 2);
  
  SkipListType::NodePtr h = s.Head();
  CHECK_EQ(h->Size(), 2);

  // Validate empty list
  using ItC = SkipListType::ItC;
  ItC itb = s.begin();
  ItC ite = s.end();
  CHECK(itb == ite);
  
  // Valudate Insert Works correctly
  s.Insert(10);
  itb = s.begin();
  ite = s.end();
  CHECK(itb != ite);
  CHECK_EQ(*itb, 10);

  // Validate iterator increments correctly
  CHECK(++itb == ite);

  // Validate Find Works correctly
  itb = s.begin();
  CHECK(s.Find(10) == itb);
  CHECK(s.Find(20) == ite);

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
