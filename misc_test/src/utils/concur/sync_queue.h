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

#ifndef _SYNC_QUEUE_H_
#define _SYNC_QUEUE_H_

//! @file   sync_queue.h
//! @brief  SyncQ: Supports workQ with multiple producers and consumers
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <iostream>         // std::cout
// C Standard Headers
// Google Headers
// Local Headers

//! @addtogroup utils
//! @{

namespace asarcar {
//-----------------------------------------------------------------------------

//! @class    SyncQ
//! @brief    Supports workQ with multiple producers and consumers
template <typename T>
class SyncMsgQ {
 public:
  SyncMsgQ()                             = default;
  ~SyncMsgQ()                            = default;
  SyncMsgQ(const SyncMsgQ& o)            = delete;
  SyncMsgQ& operator=(const SyncMsgQ& o) = delete;
  SyncMsgQ(SyncMsgQ&& o): 
      _cv{std::move(o._cv)}, _m{std::move(o._m)}, _q{std::move(o._q)} {}
  SyncMsgQ& operator=(SyncMsgQ&& o) {
    _cv = std::move(o._cv);
    _m = std::move(o._m);
    _q = std::move(o._q);
  }
  void push(T&& val) {
    {
      std::lock_guard<std::mutex> lck{_m};
      _q.push_back(std::move(val));
    }
    // unlocked before notify so that we avoid unnecessary locking of the woken task
    _cv.notify_one();
  }
  T pop(void) {
    std::unique_lock<std::mutex> lck{_m};
    _cv.wait(lck, [this](){return !this->_q.empty();});
    T res = std::move(_q.front());
    _q.pop_front();
    return res;
  }
 private:
  std::condition_variable _cv;
  std::mutex              _m;
  std::deque<T>           _q;  
};    

//-----------------------------------------------------------------------------
} // namespace asarcar

#endif // _SYNC_QUEUE_H_
