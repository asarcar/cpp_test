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
#include <climits>                  // CHAR_BIT
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/ds/radix_trie.h"
#include "utils/ds/string_prefix.h"
#include "utils/nwk/ipv4_prefix.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::nwk;
using namespace asarcar::utils::ds;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

#define CHECK_STRINGEQ(str, cstr) CHECK_STREQ((str).c_str(), (cstr))

class RadixTrieTester {
 public:
  using ItPrefix = ItR<IPv4Prefix,string>;
  using ItString = ItR<StringPrefix,string>;

  void RouteTest(void);
  void StringTest(void);

 private:
  RadixTrie<IPv4Prefix, string> rtpref_;
  RadixTrie<StringPrefix, string>     rtstr_;

  void AddRoute(const char pref[], int len, const char value[]) {
    IPv4Prefix rt{pref, len};
    rtpref_[rt] = value;
  }

  const string GetNH(const char *pref, int len) {
    ItPrefix it = rtpref_.Find(IPv4Prefix{pref, len});
    ItPrefix ite = rtpref_.End();
    if (it == ite) 
      return "";
    return it->second.c_str();
  }

  const string LPMGetRoute(const char *pref, int len=0) {
    DCHECK(len < IPv4::MAX_LEN);
    len = (len == 0) ? IPv4::MAX_LEN : len;
    ItPrefix it = rtpref_.LongestPrefixMatch(IPv4Prefix{pref, len});
    ItPrefix ite = rtpref_.End();
    if (it == ite) 
      return "";
    DLOG(INFO) << "Key=" << it->first << ": Value=" << it->second;
    return it->first.to_string();
  }

  const string EraseRoute(const char* pref, int len) {
    IPv4Prefix prefix{pref,len};
    ItPrefix it = rtpref_.Find(prefix);
    if (it == rtpref_.End())
      return "";
    ItPrefix itnext = rtpref_.Erase(it);
    if (itnext == rtpref_.End())
      return "";

    return itnext->first.to_string();
  }
};

void RadixTrieTester::RouteTest(void) {
  AddRoute("10.0.0.0",     8, "10.7.22.2");
  AddRoute("0.0.0.0",      0, "10.7.22.1");
  AddRoute("172.16.0.0",  16, "10.7.22.3");  
  AddRoute("172.17.0.0",  16, "10.7.22.4");
  AddRoute("172.18.0.0",  16, "10.7.22.5");
  AddRoute("172.19.0.0",  16, "10.7.22.6");
  AddRoute("192.168.1.0", 24, "10.7.22.7");
  AddRoute("192.168.2.0", 24, "10.7.22.8");
  AddRoute("192.168.3.0", 24, "10.7.22.9");
  AddRoute("192.168.4.0", 24, "10.7.22.10");
  DLOG(INFO) << "AddRoutes Done. IPv4 Radix Trie Debug Dump: " << rtpref_.to_string(true);

  CHECK_STRINGEQ(GetNH("0.0.0.0", 0), "10.7.22.1");
  CHECK_STRINGEQ(GetNH("10.0.0.0", 8), "10.7.22.2");
  CHECK_STRINGEQ(GetNH("172.16.0.0", 16), "10.7.22.3");
  CHECK_STRINGEQ(GetNH("172.17.0.0", 16), "10.7.22.4");
  CHECK_STRINGEQ(GetNH("172.18.0.0", 16), "10.7.22.5");
  CHECK_STRINGEQ(GetNH("172.19.0.0", 16), "10.7.22.6");
  CHECK_STRINGEQ(GetNH("192.168.1.0", 24), "10.7.22.7");
  CHECK_STRINGEQ(GetNH("192.168.2.0", 24), "10.7.22.8");
  CHECK_STRINGEQ(GetNH("192.168.3.0", 24), "10.7.22.9");
  CHECK_STRINGEQ(GetNH("192.168.4.0", 24), "10.7.22.10");

  CHECK_STRINGEQ(LPMGetRoute("172.32.1.10"), "0.0.0.0/0");
  CHECK_STRINGEQ(LPMGetRoute("172.16.3.1"), "172.16.0.0/16");
  CHECK_STRINGEQ(LPMGetRoute("192.168.1.10"), "192.168.1.0/24");
  CHECK_STRINGEQ(LPMGetRoute("192.168.2.220", 16), "0.0.0.0/0");
  CHECK_STRINGEQ(LPMGetRoute("10.16.0.0", 16), "10.0.0.0/8");

  
  CHECK_STRINGEQ(EraseRoute("10.0.0.0", 8), "172.16.0.0/16");
  CHECK_STRINGEQ(LPMGetRoute("10.1.1.1"), "0.0.0.0/0");
  CHECK_STRINGEQ(EraseRoute("172.18.0.0", 16), "172.19.0.0/16");
  CHECK_STRINGEQ(EraseRoute("172.19.0.0", 16), "192.168.1.0/24");
  CHECK_STRINGEQ(EraseRoute("192.168.4.0", 24), "");
  DLOG(INFO) << "EraseRoute: 10/8, 172.18/16, 172.19/16, and 192.168.4/24 Done.";
  DLOG(INFO) << "IPv4 Radix Trie: " << rtpref_;
  CHECK_STRINGEQ(EraseRoute("192.168.1.0", 24), "192.168.2.0/24");
  CHECK_STRINGEQ(EraseRoute("192.168.2.0", 24), "192.168.3.0/24");
  CHECK_STRINGEQ(EraseRoute("192.168.3.0", 24), "");
  CHECK_STRINGEQ(EraseRoute("172.16.0.0", 16), "172.17.0.0/16");
  CHECK_STRINGEQ(EraseRoute("172.17.0.0", 16), "");
  CHECK_EQ(rtpref_.NSize(), 1);
  CHECK_STRINGEQ(EraseRoute("0.0.0.0", 0), "");
  CHECK_EQ(rtpref_.Size(), 0);
  
  DLOG(INFO) << "EraseRoute: All prefixes done. IPv4 Radix Trie: " << rtpref_;
}
              
void RadixTrieTester::StringTest(void) {
  // Add
  rtstr_["Biswanath Dadu"] = "01-01-01";

  rtstr_[""]                = "00-00-00";

  rtstr_["Alpana Amma"]     = "02-02-02";

  rtstr_["Arijit Baba"]     = "03-03-03";
  rtstr_["Dipannita Ma"]    = "04-04-04";
  rtstr_["Abhineet Dada"]   = "05-05-05";

  rtstr_["Aditya Me"]       = "06-06-06";
  rtstr_["Arijit Baba Name"] = "07-07-07";
  rtstr_["Arijit"]          = "08-08-08";
  DLOG(INFO) << "String Radix Trie: " << rtstr_;

  // Longest Match
  ItString it = rtstr_.LongestPrefixMatch("Arijit B");
  string lpstr = ((it != rtstr_.End()) ? 
      ("<"+it->first.to_string()+","+it->second+">"):"not found");
  DLOG(INFO) << "Longest Prefix Match \"Arijit B\"=" << lpstr;
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
