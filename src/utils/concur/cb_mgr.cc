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
#include <thread>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/concur/cb_mgr.h"
#include "utils/concur/cv_guard.h"
#include "utils/concur/lock_guard.h"
#include "utils/concur/spin_lock.h"
#include "utils/ds/elist.h"

using namespace std;
using namespace ::asarcar::utils::ds;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------
struct CbMgr::CbState {
  using Ptr = shared_ptr<CbState>;
  struct CbTrack; // Forward Declaration
  using CbElem = pair<thread::id, vector<CbTrack*>>;

  CbState(): sl{}, cv{sl}, quash{false}, cbs_num{0}, elist{} {}
  ~CbState()                           = default;
  CbState(const CbState& o)            = delete;
  CbState& operator=(const CbState& o) = delete;
  CbState(CbState&& o)                 = delete; 
  CbState& operator=(CbState&& o)      = delete;

  int PendingCbs(int *tot_cbs);

  SpinLock      sl;
  CV<SpinLock>  cv;
  atomic_bool   quash;   
  int           cbs_num;
  Elist<CbElem> elist;
};

//--------------------------------------------------
//-- CbMgr::CbState::CbTrack Definition & Methods --
//--------------------------------------------------
struct CbMgr::CbState::CbTrack {
  explicit CbTrack(CbMgr::CbState* cb_p) : state_p{cb_p} {
    thread::id my_tid = this_thread::get_id();
    LockGuard<SpinLock> lkg{state_p->sl};
    ++state_p->cbs_num;
    auto it = state_p->elist.find([my_tid](CbElem& e){return my_tid==e.first;});
    // enter the tracker at the top of the stack of callbacks 
    if (it != state_p->elist.end()) {
      it->second.emplace_back(this);
      return;
    }
    state_p->elist.emplace_back(CbElem{my_tid, vector<CbTrack*>{this}});
    return;
  }
  ~CbTrack() {
    thread::id my_tid = this_thread::get_id();
    CV<SpinLock>::SignalGuard sg{state_p->cv};
    auto it = state_p->elist.find([my_tid](CbElem& e){return my_tid==e.first;});
    DCHECK(it != state_p->elist.end());
    DCHECK_GE(it->second.size(), 1);
    DCHECK_EQ(it->second.back(), this);
    --state_p->cbs_num;
    if (it->second.size() == 1) {
    state_p->elist.erase(it);
    return;
    }
    it->second.pop_back();
    return;
  }
  // CbState object lifetime beyond CbTrack: keep raw ptr
  typename CbMgr::CbState* state_p; 
};

//-------------------
//-- CbMgr Methods --
//-------------------
CbMgr::CbMgr(): cb_p_{make_shared<CbState>()} {}

void CbMgr::SealedCb(CbStatePtr& cb_p, const Closure& naked_cb) {
  if (cb_p->quash)
    return;
  CbState::CbTrack cbt{cb_p.get()};
  naked_cb();
}

void CbMgr::QuashNWait(const char* file_name, int line_num) {
  // memory_order is sequential_consistent
  cb_p_->quash = true; 
  Clock::TimePoint begin = Clock::USecs();
  int tot_cbs;
  // current thread context is executing: my_cbs won't change in wait
  int my_cbs = cb_p_->PendingCbs(&tot_cbs); 
  {
    CV<SpinLock>::WaitGuard{cb_p_->cv,
          [this,my_cbs](){return my_cbs==cb_p_->cbs_num;}};
  }
  DLOG(INFO) << __FUNCTION__ << ": Total CBs=" << tot_cbs
             << ": My CBs=" << my_cbs 
             << " pending in current thread - waited for " 
             << Clock::USecs() - begin 
             << " usecs to complete pending callbacks in other threads.";
}

//----------------------------
//-- CbMgr::CbState Methods --
//----------------------------
int CbMgr::CbState::PendingCbs(int *tot_cbs_p) {
  thread::id my_tid = this_thread::get_id();
  LockGuard<SpinLock> lkg{sl};
  *tot_cbs_p = cbs_num;
  auto it = elist.find([my_tid](CbElem& e){return my_tid==e.first;});
  return (it == elist.end()) ? 0: it->second.size();
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
