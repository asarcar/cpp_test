// Copyright 2015 asarcar Inc.
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
#include <atomic>           // std::atomic_int
#include <iostream>         // std::cout
#include <mutex>            // std::mutex
#include <thread>           // std::thread
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/concur/concur_q.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::concur;

// Declarations
DECLARE_bool(auto_test);
DECLARE_bool(benchmark);

template <typename ValueType>
class ConcurQMutex {
 public:
  using ValueTypePtr = std::unique_ptr<ValueType>;
 private:
  struct Node {
    Node(ValueTypePtr&& val) : val_{std::move(val)}, next_{nullptr} {}
    ValueTypePtr  val_;
    Node*  next_;
  };
 public:
  ConcurQMutex(): lck_{} {
    // create sentinel object
    head_ = tail_ = new Node(ValueTypePtr{});
  }
  ~ConcurQMutex() {
    Node *tmpn;
    for (Node *tmp = head_; tmp != nullptr; tmp = tmpn) {
      tmpn = tmp->next_;
      delete tmp->val_.release();
      delete tmp;
    }
  }

  void Push(ValueTypePtr&& val) {
    Node* np = new Node(std::move(val));
    lock_guard<std::mutex> lg{lck_};
    tail_->next_ = np;
    tail_        = np;
    return;
  }

  ValueTypePtr Pop(void) {
    Node*         prev_head;
    ValueTypePtr  val{nullptr};
    {
      // protect all consumers from critical region
      lock_guard<std::mutex> lg{lck_};
      Node* new_head = head_->next_;
      if (new_head == nullptr)
        return val;
      prev_head = head_;
      head_ = new_head;
      val.swap(head_->val_); // val.reset(sentinel_->val_.release())
    }
    delete prev_head;
    return val; 
  }

 private:
  std::mutex        lck_;
  Node*             head_;
  Node*             tail_;
};

constexpr static size_t ElemSize1 = 4;
constexpr static size_t ElemSize2 = 1024;

template <size_t ElemSize>
class ConcurQTester {
 public:
  class Elem {
   public:
    static_assert(ElemSize >= 4, "Minimum object size is 4 bytes");
    Elem(int num=0) : 
        v{static_cast<uint8_t>(num & 0xff), 
          static_cast<uint8_t>((num >> 8) & 0xff), 
          static_cast<uint8_t>((num >> 16) & 0xff), 
          static_cast<uint8_t>((num >> 24) & 0xff)} {}
    operator int() {
      return (v.at(0) | v.at(1) << 8 | v.at(2) << 16 | v.at(3) << 24); 
    }
   private:
    std::array<uint8_t, ElemSize> v;
  };
  using ElemPtr = std::unique_ptr<Elem>;

  ConcurQTester()  = default;
  ~ConcurQTester() = default;

  void SanityTest();
  void BasicTest();
  void ProduceStressTest();
  void ConsumeStressTest();
  void ProduceConsumeStressTest();
  void BenchmarkTest();

 private:
  static constexpr const char* kUnitStr = "us";
  static constexpr uint32_t    kSleepDuration = 10000;
  static constexpr uint32_t    kNumProduceConsume = 10000;

  ConcurQ<Elem>       cq_{};
  ConcurQMutex<Elem>  cqm_{}; 
};

template <size_t ElemSize>
constexpr const char* ConcurQTester<ElemSize>::kUnitStr;
template <size_t ElemSize>
constexpr uint32_t    ConcurQTester<ElemSize>::kSleepDuration;
template <size_t ElemSize>
constexpr uint32_t    ConcurQTester<ElemSize>::kNumProduceConsume;

// 1. Pop on empty Q returns nullptr
// 2. Push an entry in Q. 
//    Pop returns the same entry from Q. 
//    Another Pop returns nullptr.
// 3. Push two entries in Q. 
//    Pop returns same entries in same order.
// 4. Run steps 1 to 3 on ConcurQMutex as well.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::SanityTest() {
  {
    // 1
    CHECK(cq_.Pop().get() == nullptr);
    // 2
    Elem* p1 = new Elem{};
    cq_.Push(ElemPtr{p1});
    ElemPtr up1 = cq_.Pop();
    CHECK(up1.get() == p1);
    // 3
    cq_.Push(std::move(up1));
    CHECK(up1.get() == nullptr);
    Elem* p2 = new Elem{};
    cq_.Push(ElemPtr{p2});
    up1 = cq_.Pop();
    CHECK(up1.get() == p1);
    CHECK(cq_.Pop().get() == p2);

    CHECK(cq_.Pop().get() == nullptr);
  }
  // 4
  {
    // 1
    CHECK(cqm_.Pop().get() == nullptr);
    // 2
    Elem* p1 = new Elem{};
    cqm_.Push(ElemPtr{p1});
    ElemPtr up1 = cqm_.Pop();
    CHECK(up1.get() == p1);
    // 3
    cqm_.Push(std::move(up1));
    CHECK(up1.get() == nullptr);
    Elem* p2 = new Elem{};
    cqm_.Push(ElemPtr{p2});
    up1 = cqm_.Pop();
    CHECK(up1.get() == p1);
    CHECK(cqm_.Pop().get() == p2);

    CHECK(cqm_.Pop().get() == nullptr);
  }
}

// 1. Producer Threads produce from 1 to NUM_ELEMS, 
//    NUM_ELEMS+1 to 2*NUM_ELEMS, etc.
// 2. Consumer Threads consume numbers.
// 3. Validate all numbers produced are consumed by matching the sum
template <size_t ElemSize>
void ConcurQTester<ElemSize>::BasicTest() {
  constexpr int NUM_PROS  = 2;
  constexpr int NUM_CONS  = 2;
  constexpr int NUM_ELEMS = 5;
  
  std::array<std::thread, NUM_PROS+NUM_CONS> ths;
  std::atomic_int val{0};
  int  val_expected=0;

  for (int i=0; i<NUM_PROS;++i) {
    for (int j=i*NUM_ELEMS; j<(i+1)*NUM_ELEMS; ++j)
      val_expected += j+1;
  }

  for (int i=0; i<NUM_PROS;++i) {
    ths[i] = std::thread([this, i](){
        for (int j=i*NUM_ELEMS; j<(i+1)*NUM_ELEMS; ++j)
          cq_.Push(ElemPtr{new Elem(j+1)});
      });
  }
  for (int i=NUM_PROS; i<NUM_PROS+NUM_CONS;++i) {
    ths[i] = std::thread([this, val_expected](std::atomic_int& v){
        // loop until all the numbers produced are not consumed
        for(;;) {
          int v_acc = v.load();
          ElemPtr p = cq_.Pop();
          CHECK_LE(v_acc, val_expected);
          if (p) {
            int value = *p;
            v.fetch_add(value);
          } else if (v_acc == val_expected) {
            return;
          }
        }
      }, std::ref(val));
  }

  // All producers and consumers join with the mainline threads
  for (int i=0; i<NUM_PROS+NUM_CONS;++i)
    ths[i].join();

  CHECK_EQ(val.load(), val_expected);
}

template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceStressTest() {
}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ConsumeStressTest() {}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceConsumeStressTest() {}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::BenchmarkTest() {}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  ConcurQTester<ElemSize1> test{};
  test.SanityTest();
  test.BasicTest();
  test.ProduceStressTest();
  test.ConsumeStressTest();
  test.ProduceConsumeStressTest();
  if (FLAGS_benchmark)
    test.BenchmarkTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_bool(benchmark, false, 
            "test run when benchmarking spinlock Lock/Unlock"); 
