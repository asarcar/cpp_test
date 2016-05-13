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
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <array>
#include <atomic>
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>
// Local Headers
#include "utils/basic/init.h"
#include "utils/concur/concur_hash.h"
#include "utils/concur/thread_pool.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

static constexpr int kNumThreads = 4;

class String {
 public:
  String(string s): i_{atoi(s.c_str())}, s_{s} {}
  String(int i): i_{i}, s_{to_string(i)} {}
  operator int() const {
    return i_;
  }
  operator string() const {
    return s_;
  }
 private:
  int i_;
  string s_;
};

namespace std {
template <>
struct hash<String> {
  using argument_type = String;
  using result_type   = size_t;
  result_type operator()(argument_type const& s) const noexcept {
    return hash<string>{}(string(s));
  }
};
}

template <typename T>
class ConcurHashTester {
 public:
  ConcurHashTester(): cmap_{}, size_{0} {}
  void SanityTest(void);
  void ConcurSimpleTest(void);
  void ConcurStressTest(void);
 private:
  using MapType   = ConcurHash<T,T>;
  using ValPtr    = typename MapType::ValuePtr;

  MapType      cmap_;  
  atomic_int   size_;

  static constexpr int kMaxBits    = 6;
  static constexpr int kLessBits   = 3;
  static constexpr int kMaxVal     = 1 << kMaxBits;
  static constexpr int kNumThreads = 4; 
  static void SameKeyOp(ConcurHashTester *p, const T& k, const int threadNum);
  static void InsertOp(ConcurHashTester *p);
  static void EraseOp(ConcurHashTester *p);
};

template <typename T>
constexpr int ConcurHashTester<T>::kNumThreads;

template <typename T>
void ConcurHashTester<T>::SanityTest(void) {
  CHECK(cmap_.Find(1) == nullptr);

  CHECK_EQ(*cmap_.Insert(1, 2), 2);
  CHECK_EQ(*cmap_.Insert(3, 4), 4);
  CHECK_EQ(*cmap_.Find(1), 2);
  CHECK_EQ(cmap_.Size(), 2);
  
  CHECK(cmap_.Insert(1, 3) == nullptr);
  CHECK(cmap_.Erase(1));
  CHECK(!cmap_.Erase(1));

  cmap_.Clear();
  CHECK_EQ(cmap_.Size(), 0);
}

template <typename T>
void ConcurHashTester<T>::ConcurSimpleTest(void) {
  auto tpool_p = make_shared<ThreadPool<>>(kNumThreads);
  for (int i=0; i<kNumThreads; ++i)
    tpool_p->AddTask(bind(&SameKeyOp, this, T{5}, i));
  tpool_p.reset();

  CHECK_EQ(cmap_.Size(),0);
}

template <typename T>
void ConcurHashTester<T>::ConcurStressTest(void) {
  auto tpool_p = make_shared<ThreadPool<>>(kNumThreads);
  for (int i=0; i<kNumThreads/2; ++i) {
    tpool_p->AddTask(bind(&InsertOp, this));
    tpool_p->AddTask(bind(&EraseOp, this));
  }
  tpool_p.reset();

  CHECK_EQ(size_, cmap_.Size());
}

template <typename T>
void ConcurHashTester<T>::SameKeyOp(ConcurHashTester *p, 
                                    const T& k, 
                                    const int threadNum) {
  int val, cur_val; 
  for (int i=0; i<kMaxVal; ++i) {
    cur_val = i + kMaxVal*threadNum;
    ValPtr vp = p->cmap_.Insert(T{k}, T{cur_val});
    if (vp == nullptr) // inserted by another thread
      continue;
    val = *vp;
    CHECK_EQ(val, cur_val); 
    ValPtr vp2 = p->cmap_.Find(k);
    if (vp2 == nullptr) // key deleted by another thread
      continue;
    val = *vp2;
    CHECK(val >= 0 && val < kMaxVal*kNumThreads);
    CHECK(val == cur_val || 
          val < kMaxVal*threadNum || 
          val >= kMaxVal*(threadNum+1));
    p->cmap_.Erase(k);
  }
}

template <typename T>
void ConcurHashTester<T>::InsertOp(ConcurHashTester *p) {
  for (int i=0; i<kMaxVal; ++i) {
    ValPtr vp = p->cmap_.Insert(T{i >> (kMaxBits - kLessBits)}, T{i});
    if (vp == nullptr)
      continue;
    ++p->size_;
  }
}

template <typename T>
void ConcurHashTester<T>::EraseOp(ConcurHashTester *p) {
  for (int i=0; i<kMaxVal; ++i) {
    if (p->cmap_.Erase(T{i >> (kMaxBits - kLessBits)}))
      --p->size_;
  }
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  ConcurHashTester<int> cht;
  cht.SanityTest();
  LOG(INFO) << "ConcurrentHash: POD(K/V: Int) Key/Val Sanity Test Passed";
  cht.ConcurSimpleTest();
  LOG(INFO) << "ConcurrentHash: POD(K/V: Int) Key/Val "
            << "Concurrent Simple Test Passed";
  cht.ConcurStressTest();
  LOG(INFO) << "ConcurrentHash: POD(K/V: Int) Key/Val "
            << "Concurrent Stress Test Passed";
  ConcurHashTester<String> chts;
  chts.SanityTest();
  LOG(INFO) << "ConcurrentHash: Complex(K/V: {int,string}) Key/Val "
            << "Sanity Test Passed";
  chts.ConcurSimpleTest();
  LOG(INFO) << "ConcurrentHash: Complex(K/V: {int,string}) Key/Val "
            << "Concurrent Simple Test Passed";
  chts.ConcurStressTest();
  LOG(INFO) << "ConcurrentHash: Complex(K/V: {int,string}) Key/Val "
            << "Concurrent Stress Test Passed";

  if (FLAGS_auto_test)
    return 0;

  // Performance Test

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
