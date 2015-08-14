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
#include <algorithm>            // std::find_if
#include <condition_variable>   // std::condition_variable
#include <deque>                // std::deque
#include <functional>           // std::equal_to
#include <iostream>             // std::cout
#include <mutex>                // std::mutex
#include <thread>               // std::thread

// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/concur/rw_mutex.h"

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
void rw_mutex::lock(LockMode mode) {
  FASSERT(mode != LockMode::UNLOCK);
  std::thread::id my_th_id = std::this_thread::get_id();

  std::unique_lock<std::mutex> lck{_mutex};
  // Add the thread to the queue of pending requests waiting for the lock
  _q_pending.push_back(my_th_id);
  _cv.wait(lck, [this, mode, my_th_id]() {
      // A thread is allowed to own the lock when the following are true:
      // 1. You are at the head of the queue. In order to prevent WR starvation
      //    as RD always get through all the pending requests are queued up in
      //    a double ended queue and locks are ONLY granted in that order. Note 
      //    that this ensures fairness at the cost of throughput (i.e. we may not 
      //    allow RD requests behind a WR request to progress ahead even though
      //    one could have satisfied the RD request).
      // 2.a. No thread is owning the lock is empty. So either WR or RD request
      //      may be satisfied OR
      // 2.b. Current thread owning the lock is RD request and pending request
      //      is RD as well. Both can simultaneously enter the critical region.
      return ((this->_q_pending.front() == my_th_id) &&
              ((this->_cur_mode == LockMode::UNLOCK) || 
               (this->_cur_mode == LockMode::SHARE_LOCK && 
                mode == LockMode::SHARE_LOCK)));
    });

  FASSERT(this->_q_pending.front() == my_th_id);
  _q_pending.pop_front();
  _v_owners.push_back(my_th_id);

  if (mode == LockMode::EXCLUSIVE_LOCK) {
    FASSERT(this->_num_readers == 0);
    this->_cur_mode = LockMode::EXCLUSIVE_LOCK;
  } else {
    this->_num_readers++;
    this->_cur_mode = LockMode::SHARE_LOCK;
  }

  LOG(INFO) << "rw_lock state =>" << *this << "<=" << std::endl;

  return; 
}

void rw_mutex::unlock(void) {
  std::thread::id my_th_id = std::this_thread::get_id();

  { // Begin: Critical Region
    std::lock_guard<std::mutex> lck{_mutex};
    if (_cur_mode == LockMode::SHARE_LOCK) {
      FASSERT(_num_readers >= 1);
      _num_readers--;
      if (_num_readers == 0)
        _cur_mode = LockMode::UNLOCK;
    } else {
      _cur_mode = LockMode::UNLOCK;
    }
    std::remove_if(_v_owners.begin(), _v_owners.end(), 
                   [&my_th_id](const std::thread::id& th_id){return (my_th_id == th_id);});
  } // End: Critical Region

  // unlocked before notify: avoids unnecessary locking of the woken task
  _cv.notify_all();
  return;
}

// TODO bool rw_mutex::try_lock(LockMode mode);

std::ostream& operator<<(std::ostream& os, const rw_mutex& rwm) {
  os << "cur_mode " << rwm._cur_mode
     << ": num_readers " << rwm._num_readers;
  os << ": pending threads [ " << std::hex;
  for (auto &id : rwm._q_pending)
    os << "0x" << id << " ";
  os << "]";
  os << ": owners [ ";
  for (auto &id : rwm._v_owners)
    os << "0x" << id << " ";
  os << "]";
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

