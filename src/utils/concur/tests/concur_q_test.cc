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
#include "utils/basic/clock.h"
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
  static constexpr const char* kUnitStr   = "us";
  static constexpr int kSleepDuration     = 10000;
  static constexpr int kNumElemsLow       = 5;
  static constexpr int kNumElemsMid       = 100;
  static constexpr int kNumElemsHigh      = 800;
  static constexpr int kNumElemsStress    = 1000;
  static constexpr int kNumThsLow         = 1;
  static constexpr int kNumThsHigh        = 2;
  static constexpr int kNumThsStress      = 8;



  ConcurQ<Elem>       cq_{};
  ConcurQMutex<Elem>  cqm_{}; 

  template <typename QueueType>
  void HelperSanityTest(QueueType& q);

  template <int NumElems, int NumProducers, 
            int NumConsumers, typename QueueType>
  void HelperStressTest(QueueType& q);
};

template <size_t ElemSize>
constexpr const char* ConcurQTester<ElemSize>::kUnitStr;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kSleepDuration;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumElemsLow;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumElemsMid;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumElemsHigh;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumElemsStress;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumThsLow;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumThsHigh;
template <size_t ElemSize>
constexpr int ConcurQTester<ElemSize>::kNumThsStress;

// 1. Pop on empty Q returns nullptr
// 2. Push an entry in Q. 
//    Pop returns the same entry from Q. 
//    Another Pop returns nullptr.
// 3. Push two entries in Q. 
//    Pop returns same entries in same order.
template <size_t ElemSize>
template <typename QueueType>
void ConcurQTester<ElemSize>::HelperSanityTest(QueueType& q) {
  // 1
  CHECK(q.Pop().get() == nullptr);
  // 2
  Elem* p1 = new Elem{};
  q.Push(ElemPtr{p1});
  ElemPtr up1 = q.Pop();
  CHECK(up1.get() == p1);
  // 3
  q.Push(std::move(up1));
  CHECK(up1.get() == nullptr);
  Elem* p2 = new Elem{};
  q.Push(ElemPtr{p2});
  up1 = q.Pop();
  CHECK(up1.get() == p1);
  CHECK(q.Pop().get() == p2);
  
  CHECK(q.Pop().get() == nullptr);
}

// Run SanityTest on ConcurQ and ConcurQMutex as well.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::SanityTest() {
  HelperSanityTest(cq_);
  HelperSanityTest(cqm_);
}

// 1. Producer Thread One produce from 1 to NUM_ELEMS, 
//    Thread Two produces NUM_ELEMS+1 to 2*NUM_ELEMS, and so on.
// 2. Consumer Thread One to NumConsumers consume any number available.
// 3. Validate all numbers produced are consumed by matching the sum
template <size_t ElemSize>
template <int NumElems, int NumProducers, 
          int NumConsumers, typename QueueType>
void ConcurQTester<ElemSize>::HelperStressTest(QueueType& q) {
  std::array<std::thread, NumProducers+NumConsumers> ths;
  std::atomic_int val{0};
  int  val_expected=0;

  for (int i=0; i<NumProducers;++i) {
    for (int j=i*NumElems; j<(i+1)*NumElems; ++j)
      val_expected += j+1;
  }

  for (int i=0; i<NumProducers;++i) {
    ths[i] = std::thread([this, i, &q](){
        for (int j=i*NumElems; j<(i+1)*NumElems; ++j)
          q.Push(ElemPtr{new Elem(j+1)});
      });
  }
  for (int i=NumProducers; i<NumProducers+NumConsumers;++i) {
    ths[i] = std::thread([this, val_expected, &q](std::atomic_int& v){
        // loop until all the numbers produced are not consumed
        for(;;) {
          int v_acc = v.load();
          ElemPtr p = q.Pop();
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
  for (int i=0; i<NumProducers+NumConsumers;++i)
    ths[i].join();

  CHECK_EQ(val.load(), val_expected);
}

// Low# producers produce numbers from 1 to Low# of Elems
// and linearly increase the value for different threads
// Low# consumers consume the numbers produced.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::BasicTest() {
  HelperStressTest<kNumElemsLow, kNumThsLow, kNumThsLow>(cq_);
}

// High# of producers produce numbers from 1 to Mid# of Elems
// and linearly increase the value for different threads
// Low# of consumers consumes the numbers produced.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceStressTest() {
  HelperStressTest<kNumElemsMid, kNumThsHigh, kNumThsLow>(cq_);
}

// Low# of producers produce numbers from 1 to High# of Elems
// and linearly increase the value for different threads
// High# of consumers consumes the numbers produced.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ConsumeStressTest() {
  HelperStressTest<kNumElemsHigh, kNumThsLow, kNumThsHigh>(cq_);
}

// High# of producers produce numbers from 1 to Mid# of Elems
// and linearly increase the value for different threads
// High# of consumers consumes the numbers produced.
template <size_t ElemSize>
void ConcurQTester<ElemSize>::ProduceConsumeStressTest() {
  HelperStressTest<kNumElemsMid, kNumThsHigh, kNumThsHigh>(cq_);
}

// (a) Concurrent Q Case:
//   Stress# of producers produce numbers from 1 to Stress# of Elems
//   and linearly increase the value for different threads
//   Stress# of consumers consumes the numbers produced.
// (b) Mutex Case:
//   Stress# of producers produce numbers from 1 to Stress# of Elems
//   and linearly increase the value for different threads
//   Stress# of consumers consumes the numbers produced.
// Compare the two run times (a) and (b)
template <size_t ElemSize>
void ConcurQTester<ElemSize>::BenchmarkTest() {
  Clock::TimePoint    nowCQM = Clock::USecs();
  HelperStressTest<kNumElemsStress, kNumThsStress, kNumThsStress>(cqm_);
  Clock::TimeDuration durCQM = Clock::USecs() - nowCQM;

  Clock::TimePoint    nowCQ = Clock::USecs();
  HelperStressTest<kNumElemsStress, kNumThsStress, kNumThsStress>(cq_);
  Clock::TimeDuration durCQ = Clock::USecs() - nowCQ;

  LOG(INFO) << "TIME:" << std::endl
            << "#ElemSize " << ElemSize << std::endl
            << "#Elems " << kNumElemsStress << " by each producer" << std::endl
            << "#Producer Threads " << kNumThsStress << std::endl
            << "#Consumer Threads " << kNumThsStress << std::endl
            << "Mutex/SpinLock " << durCQM << "/" << durCQ << kUnitStr 
            << ": SpeedUp = " << static_cast<double>(durCQM)/static_cast<double>(durCQ);
  
  return;
}

template <typename ConQTester>
void HelperConQTester(ConQTester&& cqt) {
  cqt.SanityTest();
  cqt.BasicTest();
  cqt.ProduceStressTest();
  cqt.ConsumeStressTest();
  cqt.ProduceConsumeStressTest();  

  if (FLAGS_benchmark)
    cqt.BenchmarkTest();
}
      

int main(int argc, char *argv[]) {
  constexpr size_t kElemSizeLow  =    4;
  constexpr size_t kElemSizeHigh = 1024;

  Init::InitEnv(&argc, &argv);

  HelperConQTester(ConcurQTester<kElemSizeLow>{});
  HelperConQTester(ConcurQTester<kElemSizeHigh>{});

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
DEFINE_bool(benchmark, false, 
            "test run when benchmarking spinlock Lock/Unlock"); 
