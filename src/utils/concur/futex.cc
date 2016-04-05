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
#include <sstream>      // ostringstream
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/concur/futex.h"

using namespace std;

namespace asarcar { namespace utils { namespace concur {
//-----------------------------------------------------------------------------

// When testval == std::numeric_limits<int>::max() we want the wait to
// succeed. 
// While this is not guaranteed, the atomic operations above with 
// memory_order_seq_cst above and below the system call (SYS_futes)
// ensures that val_p_ does not change while the
// arguments are evaluated prior to system call execution.
// However, another thread may change the value of val_p_ in 
// between unless the call to wait region is protected inside a 
// lock (spin or mutex).
int Futex::Wait(int testval) {
  ++num_; // <==> num_.fetch_add(1, std::memory_order_seq_cst);
  int retval = syscall(SYS_futex, val_p_, FUTEX_WAIT_PRIVATE, 
                       ((testval == std::numeric_limits<int>::max()) ?
                        val_p_->load(): testval), 
                       NULL, NULL, 0);
  
  PCHECK(--num_ >= 0); // <=> num_.fetch_sub(1, std::memory_order_seq_cst)
  return retval;
}

int Futex::Wake(bool wake_all) {
  return syscall(SYS_futex, val_p_, FUTEX_WAKE_PRIVATE, 
                 (wake_all?std::numeric_limits<int>::max():1), 
                 NULL, NULL, 0);
}

string Futex::to_string(void) {
  ostringstream oss;
  oss << "Watch_address=" << std::hex << val_p_ << std::dec
      << ": value=" << *val_p_
      << ": num_threads_waiting=" << num_;
  return oss.str();
}

ostream& operator<<(ostream& os, Futex& f) {
  os << f.to_string();
  return os;
}

//-----------------------------------------------------------------------------
} } } // namespace asarcar { namespace utils { namespace concur {
