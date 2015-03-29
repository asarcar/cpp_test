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
#include "utils/ds/treap.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class TreapTester {
 public:
  using Tt    = Treap<const uint32_t,double>;
  using ItC   = Tt::ItC;
  using ModPr = Tt::ModPr; 

  void ZeroEntryTest(void);
  void OneEntryTest(void);
  void TwoEntryTest(void);
  void FullTest(void);
 private:
};

// Empty Tree Test
void TreapTester::ZeroEntryTest(void) {
  Tt t;

  CHECK_EQ(t.Size(), 0); // size 0
  CHECK(t.Find(1) == t.end()); // Key 1 does not exists
  ItC b = t.begin();
  CHECK(b == t.end()); // begin iterator is the end iterator

  return;
}

// One Entry Test
void TreapTester::OneEntryTest(void) {
  Tt t;

  CHECK(t.Emplace(1, 2.0).second);
  CHECK_EQ(t.Size(), 1); // size 1

  ItC b = t.begin(), e = t.end();
  ItC it = b;

  CHECK(t.Find(2) == e); // Key 2 does not exist
  CHECK(t.Find(1) == b); // Key 1 same as being

  CHECK(++it == e); // next of begin is end
  ItC it2 = t.last();
  CHECK(it2 == b); // prev of end is begin

  DLOG(INFO) << t;

  CHECK(!t.Delete(2));
  CHECK(!t.Emplace(1, 2.0).second);
  CHECK(t.Delete(1));
  CHECK_EQ(t.Size(), 0); // size 0

  CHECK_EQ(*((*(t.Emplace(2, 3.0).first)).first), 2);
  CHECK_EQ(t.Size(), 1); // size 1

  return;
}

void TreapTester::TwoEntryTest(void) {
  Tt t;
  t.Emplace(2, 2.0);
  t.Emplace(3, 3.0);
  CHECK_EQ(t.Size(), 2); // size 1

  DLOG(INFO) << t;

  ItC b = t.begin(), e = t.end(), it = b;
  CHECK_EQ(*((*it).first), 2);
  CHECK_EQ(*((*it).second), 2.0);
  ++it;
  CHECK_EQ(*((*it).first), 3);
  CHECK_EQ(*((*it).second), 3.0);

  CHECK(++it == e);

  CHECK(!t.Remove(4).second);
  CHECK_EQ(*(*(t.Remove(2).first)).second, 3);
  
  // Identify the current begin/end of the treap after modifications
  ItC it2 = t.Remove(3).first;
  ItC e2  = t.end();
  CHECK(it2 == e2);

  CHECK_EQ(t.Size(), 0);

  return;
}

void TreapTester::FullTest(void) {
  constexpr int N=10;
  using K     = uint32_t;
  using Ks    = std::array<K, N>;
  using V     = double;
  using KV    = std::pair<const K,V>;
  using KVs   = std::array<KV, N>; 
  Ks  ks  = {K{6}, K{7}, K{12}, K{15}, K{26}, K{35}, K{56}, K{65}, K{70}, K{75}};
  KVs kvs = {
    KV{70, 30.0}, 
    KV{35, 65.0}, 
    KV{6, 94.0}, 
    KV{75, 25.0}, 
    KV{26, 74.0},
    KV{15, 85.0}, 
    KV{56, 44.0},
    KV{7, 93.0}, 
    KV{12, 88.0}, 
    KV{65, 35.0} 
  };

  Tt t;
  for (int i=0; i<N; ++i) {
    KV v = kvs.at(i);
    t.Insert(v.first, v.second);
  }

  DLOG(INFO) << t;

  // Reverse Walk validating the entries in right order
  ItC l = t.last(), e = t.end();
  int j = N-1;
  for(ItC it=l; it != e; --it, --j) {
    CHECK(j >= 0 && j < N);
    CHECK((*it).first != nullptr);
    CHECK_EQ(*((*it).first), ks.at(j));
    CHECK_EQ(*((*it).second), (100.0 - ks.at(j)));
  }

  // Remove the odd elements and confirm that even elements are remaining
  for(int i=1; i<N; i+=2)
    CHECK(t.Remove(kvs.at(i).first).second);

  // Forward Walk validating the entries in right order and all are the even ones
  int k=0;
  ItC b2 = t.begin(), e2 = t.end();
  for(ItC it=b2; it != e2; ++it, k+=2) {
    CHECK(k >= 0 && k < N);
    CHECK((*it).first != nullptr);
    CHECK_EQ(*((*it).first), ks.at(k));
    CHECK_EQ(*((*it).second), (100.0 - ks.at(k)));
  }

  return;
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  TreapTester tt;
  tt.ZeroEntryTest();
  tt.OneEntryTest();
  tt.TwoEntryTest();
  tt.FullTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
