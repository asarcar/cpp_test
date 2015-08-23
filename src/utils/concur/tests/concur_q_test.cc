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
#include <iostream>         // std::cout
#include <mutex>            // std::mutex
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
        return new_head;
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

constexpr static size_t ElemSize1 = 32;
constexpr static size_t ElemSize2 = 1024;

template <size_t ElemSize>
class ConcurQTester {
 public:
  struct Elem {
    std::array<char, ElemSize> v;
  };
  using ElemPtr = std::unique_ptr<Elem>;

  ConcurQTester()  = default;
  ~ConcurQTester() = default;

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
void ConcurQTester<ElemSize>::BasicTest() {
  CHECK(cq_.Pop().get() == nullptr);
  Elem* p1 = new Elem{};
  cq_.Push(ElemPtr{p1});
  ElemPtr up1 = cq_.Pop();
  CHECK(up1.get() == p1);

  cq_.Push(std::move(up1));
  CHECK(up1.get() == nullptr);
  Elem* p2 = new Elem{};
  cq_.Push(ElemPtr{p2});

  up1 = cq_.Pop();
  CHECK(up1.get() == p1);
  CHECK(cq_.Pop().get() == p2);

  CHECK(cq_.Pop().get() == nullptr);
}

template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceStressTest() {}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ConsumeStressTest() {}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceConsumeStressTest() {}
template <size_t ElemSize>
void ConcurQTester<ElemSize>::BenchmarkTest() {}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  ConcurQTester<ElemSize1> test{};
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
