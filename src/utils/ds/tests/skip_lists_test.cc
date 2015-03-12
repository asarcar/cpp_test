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
  using SkipListType=SkipList<uint32_t,1>;
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
  constexpr int N = 3;
  using SkipListType=SkipList<uint32_t,N>;
  using ItC   = SkipListType::ItC;
  using ModPr = SkipListType::ModPr;

  SkipListType s;
  // Validate MaxLevel = Ceiling(log2(N))
  CHECK_EQ(s.MaxLevel(), 2);
  
  SkipListType::NodePtr h = s.Head();
  CHECK_EQ(h->Size(), 2);

  // Validate empty list

  ItC itb = s.begin();
  ItC ite = s.end();
  CHECK(itb == ite);
  
  // Valudate Insert Works correctly
  ModPr r = s.Emplace(10);
  CHECK_EQ(*r.first, 10); CHECK_EQ(r.second, true);
  itb = s.begin();
  CHECK(r.first == itb);
  ite = s.end();
  CHECK(itb != ite);

  // Validate iterator increments correctly
  CHECK(++itb == ite);

  // Validate Find Works correctly
  itb = s.begin();
  CHECK(s.Find(10) == itb);

  ModPr pr = s.Remove(5);
  CHECK(pr.second == false && pr.first == itb);

  pr = s.Remove(*itb);
  CHECK(pr.second == true && pr.first == ite);

  return;
}

void SkipListTester::FullTest(void) {
  constexpr int Num = 15;
  using SkipListType=SkipList<uint32_t,Num>;
  using ItC   = SkipListType::ItC;
  using ModPr = SkipListType::ModPr;

  SkipListType s;
  // Validate MaxLevel = Ceiling(log2(Num))
  CHECK_EQ(s.MaxLevel(), 4);

  constexpr int N = 10, M = 9;
  array<uint32_t, N> n {21, 10, 16, 23, 10, 81, 7, 72, 15, 44};
  array<uint32_t, M> m {7, 10, 15, 16, 21, 23, 44, 72, 81};
  for(int i=0;i<N;++i)
    s.Emplace(uint32_t{n.at(i)});

  // Check elements were added in sorted order
  ItC itb = s.begin(); 
  ItC ite = s.end(); 
  int i=0;
  for(ItC itx = itb; itx != ite; ++itx, ++i) {
    LOG(INFO) << "Node: pos=" << i << ": lvl=" << itx->Size()
              << ": val=" << itx->Value();
    for (int j=0; j < static_cast<int>(itx->Size()); j++) {
      LOG(INFO) << "  Level=" << j << ": next=" 
                << hex << "0x" << itx->Next(j) << dec;
    }
    CHECK_EQ(*itx, m.at(i));
  }

  constexpr int R = 4, T = 4;
  array<uint32_t, R> r {21, 16, 10, 44};
  array<uint32_t, T> t {23, 23, 15, 72};
  for(int i=0; i<R; ++i) {
    ModPr pr = s.Remove(r.at(i));
    CHECK(pr.second == true && pr.first != s.end() && *pr.first == t.at(i));
  }

  ModPr pr = s.Remove(81);
  CHECK(pr.second == true && pr.first == s.end());

  constexpr int U = 4;
  array<uint32_t, U> u {7, 15, 23, 72};
  itb = s.begin(); ite = s.end();
  i=0;
  for(ItC itx = itb; itx != ite; ++itx, ++i)
    CHECK_EQ(*itx, u.at(i));
         
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
