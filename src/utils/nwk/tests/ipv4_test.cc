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
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/init.h"
#include "utils/nwk/ipv4.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::nwk;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

class IPv4Test {
 public:
  IPv4Test()  = default;
  ~IPv4Test() = default;

  void   Test(void);
 private:
};

void IPv4Test::Test() {
  const char *s = "127.1.2.3";
  IPv4 a{0x7f010203};
  IPv4 b{s};
  IPv4 c{string(s)};

  CHECK(c.IsLoopbackAddress());

  CHECK(a == c);

  CHECK_EQ(a.to_string(), c.to_string());

  CHECK(a.IsSame(string(s)));
  CHECK(a.IsSame(string(s), "255.255.255.192"));
  CHECK(a.IsSame(string(s), "255.255.224.0"));
  CHECK(a.IsSame(string(s), "192.0.0.0"));
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  IPv4Test test{};
  test.Test();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
