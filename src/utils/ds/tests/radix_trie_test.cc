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
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/ds/radix_trie.h"
#include "utils/nwk/ipv4_prefix.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::nwk;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class RadixTrieTester {
 public:
  using ItPrefix = typename RadixTrie<IPv4Prefix,string>::Iterator;
  using ItString = typename RadixTrie<string,string>::Iterator;

  void RouteTest(void);
  void StringTest(void);

 private:
  RadixTrie<IPv4Prefix, string> rtpref_;
  RadixTrie<string, string>     rtstr_;

  void AddRoute(const char* pref, int len, const char* value) {
    IPv4Prefix rt{pref, len};
    rtpref_[rt] = value;
  }

  const char* GetNH(const char *pref, int len) {
    ItPrefix it = rtpref_.Find(IPv4Prefix{pref, len});
    if (it == rtpref_.End()) 
      return "";
    return it->second.c_str();
  }
  
  bool EraseRoute(const char* pref, int len) {
    return rtpref_.Erase(IPv4Prefix{pref,len});
  }
};

void RadixTrieTester::RouteTest(void) {
  AddRoute("0.0.0.0",      0, "10.7.22.1");
#if 0
  AddRoute("10.0.0.0",     8, "10.7.22.2");
  AddRoute("172.16.0.0",  16, "10.7.22.3");
  AddRoute("172.17.0.0",  16, "10.7.22.4");
  AddRoute("172.18.0.0",  16, "10.7.22.5");
  AddRoute("172.19.0.0",  16, "10.7.22.6");
  AddRoute("192.168.1.0", 24, "10.7.22.7");
  AddRoute("192.168.2.0", 24, "10.7.22.8");
  AddRoute("192.168.3.0", 24, "10.7.22.9");
  AddRoute("192.168.4.0", 24, "10.7.22.10");
#endif

  CHECK_STREQ(GetNH("0.0.0.0", 0), "10.7.22.1");
#if 0  
  CHECK_STREQ(GetNH("10.0.0.0", 8), "10.7.22.2");
  CHECK_STREQ(GetNH("172.16.0.0", 16), "10.7.22.3");
  CHECK_STREQ(GetNH("172.17.0.0", 16), "10.7.22.4");
  CHECK_STREQ(FindRoute("192.168.1.10"));
  CHECK_STREQ(FindRoute("192.168.2.220"));
  CHECK_STREQ(FindRoute("192.168.3.80"));
  CHECK_STREQ(FindRoute("192.168.4.100"));
  CHECK_STREQ(FindRoute("172.20.0.1"));
#endif
  DLOG(INFO) << "IPv4 Radix Trie: " << rtpref_;
  // remove an entry
  EraseRoute("10.0.0.0", 8);
#if 0
  FindRoute("10.1.1.1");
#endif
}
              
void RadixTrieTester::StringTest(void) {
  DLOG(INFO) << "String Radix Trie: " << rtstr_;
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  RadixTrieTester rt;

  rt.RouteTest();
  rt.StringTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
