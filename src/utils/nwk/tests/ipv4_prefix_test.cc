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
#include "utils/nwk/ipv4_prefix.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::nwk;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

class IPv4PrefixTest {
 public:
  IPv4PrefixTest()  = default;
  ~IPv4PrefixTest() = default;

  void   Test(void);
 private:
};

void IPv4PrefixTest::Test() {
  IPv4Prefix pref{"127.1.2.3", 16};
  
  CHECK(pref.size() == 16);
  CHECK((pref.substr(8, 8) == IPv4Prefix{"1.2.3.4", 8}));
  CHECK((pref.substr(0,8) + pref.substr(8,8)) == pref);
  CHECK((pref.substr(0,4) += pref.substr(4,12)) == pref);

  CHECK(!pref[0]);
  CHECK(pref[15]);

  CHECK((pref.prefix(IPv4Prefix{"127.1.3.4", 16}) == pref));
  CHECK((pref.prefix(IPv4Prefix{"127.1.3.4", 17}) == pref));
  CHECK((pref.prefix(IPv4Prefix{"127.1.3.4", 15}) == IPv4Prefix{"127.0.0.0", 15}));
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  IPv4PrefixTest test{};
  test.Test();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
