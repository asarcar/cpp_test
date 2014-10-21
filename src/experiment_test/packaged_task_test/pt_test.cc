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
#include <deque>
#include <future>      // std::future, std::packaged_task
#include <iostream>
#include <thread>      // std::thread
#include <vector>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using namespace asarcar;
using namespace std;

class PtTest {
 public:
  struct QElem {
    int arg;
    packaged_task<int(int)> pt;
    QElem(int a, packaged_task<int(int)>&& p) : arg{a}, pt{std::move(p)}{}
    ~QElem() = default;
    QElem(QElem &&o) : arg{o.arg}, pt{std::move(o.pt)}{}
    QElem& operator=(QElem &&o) = delete;
    QElem(const QElem &o) = delete;
    QElem& operator=(const QElem &o) = delete;
  };

  PtTest(int argc, char *argv[]) {
    int num;
    if ((argc != 2) || ((num = stoi(string(argv[1]))) < 2))
      throw runtime_error("Usage: pt_test num_pts");
    DLOG(INFO) << "PtTest called with num " << num;
    // C++11 bug: why does this does not work with std::function? 
    // It seems post the first instance of creation of packaged_task
    // the remaining invocations devolve to a no-op
    // std::function out_fn = ... does not work as the MoveContructor invoked
    // by packaged_task destroys the function invocation
    // To use std::function move the out_fn definition inside the for loop (so that every
    // time the function object refers to a new out_fn) and not destroyed by 
    // move constructible invocation of packaged_task...
    auto out_fn = [](int i) ->int {
      DLOG(INFO) << "arg = " << i << endl; 
      return i+10;
    };

    for (int i=1; i<=num; ++i) {
      packaged_task<int(int)> pt{out_fn};
      _fu_pool.push_back(pt.get_future());
      _pt_pool.push_back(QElem {i, move(pt)});
    }
    return;
  }
  void exec_fn(void) {
    int i=1;
    while (_pt_pool.size() > 0) {
      QElem e = std::move(_pt_pool.front());
      _pt_pool.pop_front();
      DLOG(INFO) << "packaged_task called with arg " << i;
      e.pt(e.arg);
      i++;
    }
    return;
  }
  int proc_ret(void) {
    int i=1;
    for (auto &fu:_fu_pool) {
      DLOG(INFO) << "future call # " << i;
      _sum += fu.get();
      i++;
    }
    return _sum;
  }
 private:
  int _sum = 0;
  deque<QElem> _pt_pool;
  vector<future<int>> _fu_pool;
};

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);
  try {
    PtTest pt(argc, argv);
    pt.exec_fn();
    int sum = pt.proc_ret();
    cout << "Sum = " << sum << endl;
  }
  catch(std::exception &e) {
    LOG(WARNING) << "Exception caught: " << e.what();
    return -1;
  }
  return 0;
}

