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

#ifndef _UTILS_CONCUR_RW_MUTEX_H_
#define _UTILS_CONCUR_RW_MUTEX_H_

//! @file   rw_mutex.h
//! @brief  rw_mutex: Allows multiple readers and exclusive writer.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <condition_variable>   // std::condition_variable
#include <deque>                // std::deque
#include <iostream>             // std::cout
#include <mutex>                // std::mutex
#include <thread>               // std::thread
// C Standard Headers
// Google Headers
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"

//! @addtogroup utils
//! @{

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

//! @class    rw_mutex
//! @brief    Supports multiple readers and single writer
//! behavior undefined if rw_lock is called recursively 
class rw_mutex {
 public:
  enum class RwMode : uint8_t {EMPTY=0, READ, WRITE};
  rw_mutex(void)                         = default;
  ~rw_mutex(void)                        = default;
  rw_mutex(const rw_mutex& o)            = delete;
  rw_mutex& operator=(const rw_mutex& o) = delete;
  rw_mutex(rw_mutex&& o)                 = delete; 
  rw_mutex& operator=(rw_mutex&& o)      = delete;

  void lock(RwMode mode);
  void unlock(void);
  // TODO: bool try_lock(RwMode mode);
  static std::string disp_rw_mode(RwMode mode);

  friend std::ostream& operator<<(std::ostream& os, const rw_mutex& rwm);

 private:
  std::condition_variable       _cv{};
  std::mutex                    _mutex{};
  // queue of all threads pending this lock
  std::deque<std::thread::id>   _q_pending{}; 
  // vector of all threads owning this lock
  // one may argue this should be a list for 
  // each insertion and deletion. benchmark shows that
  // vector exhibits better performance due to memory locality 
  // for small (<10K) number of elements
  std::vector<std::thread::id>  _v_owners{};
  RwMode                        _cur_mode{RwMode::EMPTY};
  int                           _num_readers{0};
};    

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {

#endif // _UTILS_CONCUR_RW_MUTEX_H_
