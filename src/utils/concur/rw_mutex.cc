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

using namespace std;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
void RWMutex::lock(LockMode mode) {
  DCHECK(mode != LockMode::UNLOCK);
  QIterator it;
  const QElem myElem{this_thread::get_id(), mode};

  std::unique_lock<std::mutex> lck{mutex_};
  // Add the thread to the queue of pending requests waiting for the lock
  q_pending_.push_back(myElem);
  cv_.wait(lck, [this, &myElem, &it]() {
      // A thread is allowed to own the lock when the following are true:
      // 1. You are at the head of the queue or you are a shared lock 
      //    request but ahead of any exclusive lock request.
      //    We do so to prevent Wr starvation as Rr always get through all the 
      //    pending requests are queued up in a double ended queue. 
      //    So shared locks ahead of exclusive lock request are granted in random
      //    order. But interim exclusive lock requests are granted in strict FIFO
      //    order. Ensures fairness at the cost of throughput (i.e. we may not 
      //    allow RD requests behind a WR request to progress ahead even though
      //    one could have satisfied the RD request).
      // 2.a. No thread is owning the lock is empty. So either WR or RD request
      //      may be satisfied OR
      // 2.b. Current thread owning the lock is RD request and pending request
      //      is RD as well. Both can simultaneously enter the critical region.
      it=this->q_pending_.begin();

      // Exclusive Lock Request:
      // Accepted only when lock available & my request is head of queue
      if (myElem.second == LockMode::EXCLUSIVE_LOCK) {
        return (this->cur_mode_ == LockMode::UNLOCK && 
                myElem.first == it->first);
      }

      // Share Lock Request: myElem.second == LockMode::SHARE_LOCK
      // lock exclusively held? => Deny my request.
      if (this->cur_mode_ == LockMode::EXCLUSIVE_LOCK)
        return false;

      // Honor if my request is ahead of any Exclusive Lock request
      for (;it!=this->q_pending_.end();++it) {
        // My request is ahead of any exclusive lock request: Accept
        if (it->first == myElem.first)
          break;
        // Exclusive request hit ahead of my request: Deny 
        if (it->second == LockMode::EXCLUSIVE_LOCK) 
          return false;
      }
      return true;
    });

  FASSERT(it != q_pending_.end());
  q_pending_.erase(it);
  v_owners_.push_back(myElem.first);

  if (mode == LockMode::EXCLUSIVE_LOCK) {
    DCHECK_EQ(num_readers_, 0);
    cur_mode_ = LockMode::EXCLUSIVE_LOCK;
  } else {
    ++num_readers_;
    cur_mode_ = LockMode::SHARE_LOCK;
  }

  return; 
}

void RWMutex::unlock(void) {
  std::thread::id my_th_id = std::this_thread::get_id();

  { // Begin: Critical Region
    std::lock_guard<std::mutex> lck{mutex_};
    if (cur_mode_ == LockMode::SHARE_LOCK) {
      FASSERT(num_readers_ >= 1);
      --num_readers_;
      if (num_readers_ == 0)
        cur_mode_ = LockMode::UNLOCK;
    } else {
      cur_mode_ = LockMode::UNLOCK;
    }
    std::remove_if(v_owners_.begin(), v_owners_.end(), 
                   [&my_th_id](const std::thread::id& th_id){return (my_th_id == th_id);});
  } // End: Critical Region

  // unlocked before notify: avoids unnecessary locking of the woken task
  cv_.notify_all();
  return;
}

// TODO bool RWMutex::try_lock(LockMode mode);

std::ostream& operator<<(std::ostream& os, const RWMutex& rwm) {
  os << "cur_mode " << rwm.cur_mode_
     << ": num_readers " << rwm.num_readers_;
  os << ": pending_q [ " << std::hex;
  for (auto &myElem : rwm.q_pending_)
    os << "{" << std::hex << myElem.first << "," << myElem.second << "} ";
  os << "]";
  os << ": owners [ ";
  for (auto &id : rwm.v_owners_)
    os << "0x" << id << " ";
  os << "]";
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

