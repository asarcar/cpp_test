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
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/ds/string_prefix.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::ds;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class StringPrefixTester {
 public:
  void SanityTest(void);
  void PlusSubstrTest(void);
  void PrefixTest(void);
 private:
};

void StringPrefixTester::SanityTest(void) {
  // Note r = 114; b = 98; r - b = 16 chars 
  StringPrefix spb{"b"};
  StringPrefix spr{"r"};
  DLOG(INFO) << "\"b\"=" << spb.to_string(true);
  DLOG(INFO) << "\"r\"=" << spr.to_string(true);
  CHECK_EQ(spb.size(), 8); // size test
  // 'b' = 98 = 0x62=0b01100010
  // operator [] test
  CHECK(!spb[0]); CHECK(spb[1]); CHECK(spb[2]); CHECK(!spb[3]); 
  CHECK(!spb[4]); CHECK(!spb[5]); CHECK(spb[6]); CHECK(!spb[7]); 
  CHECK(spb != spr); // operator != test

  spb.resize(3); // resize test
  DLOG(INFO) << "\"b\".resize(3)=" << spb.to_string(true);
  spr.resize(3);
  DLOG(INFO) << "\"r\".resize(3)=" << spr.to_string(true);
  CHECK_EQ(spr.size(), 3);
  CHECK(spb == spr); // operator == test

  spr = StringPrefix{"r"};
  StringPrefix spr1 = spr.substr(3, 3); // substr test 
  StringPrefix spr2 = spr.substr(6, 2);
  StringPrefix spr12 = spr1 + spr2;
  DLOG(INFO) << "\"r\".substr(3,3) + \"r\".substr(6,2) i.e. " 
             << spr1.to_string(true) << "+" << spr2.to_string(true) 
             << "=" << spr12.to_string(true);
  CHECK(spr12 == spr.substr(3,5));
  spb += spr1 + spr2;
  DLOG(INFO) << "\"b\".resize(3) + \"r\".substr(3,3) + \"r\".substr(6,2)=" 
             << spb.to_string(true);
  CHECK(spb == spr);

  StringPrefix res = StringPrefix{"b"}.prefix(spr);
  DLOG(INFO) << "\"b\".prefix(\"r\")=" << res.to_string(true);
  CHECK_EQ(res.size(), 3);
}

void StringPrefixTester::PlusSubstrTest(void) {
  StringPrefix atof = StringPrefix{"abcdef"};
  StringPrefix gtol = StringPrefix{"ghijkl"};
  StringPrefix atol = atof + gtol;
  DLOG(INFO) << "atof=" << atof << ": gtol=" << gtol
             << ": atol=" << atol;
  CHECK(atol == StringPrefix{"abcdefghijkl"});
  StringPrefix atofsubstr{atof.substr(0,24)};
  atofsubstr += gtol.substr(24,24);
  DLOG(INFO) << "atof.substr(0,24)=" << atof.substr(0,24) 
             << ": gtol.substr(24,24)=" << gtol.substr(24,24)
             << ": sum=" << atofsubstr;
  CHECK(atofsubstr == StringPrefix{"abcjkl"});
  atofsubstr = atof.substr(10,10) + atof.substr(20,20);
  CHECK(atofsubstr == atof.substr(10,30));
  StringPrefix atof2 = atof.substr(0, 10) + atofsubstr;
  StringPrefix atof3 = atof2 + atof.substr(40, 8);
  DLOG(INFO) << "atof.substr(0,10)=" << atof.substr(0,10).to_string(true) 
             << ": atofsubstr(10,30)=" << atofsubstr.to_string(true)
             << ": sum so far=" << atof2.to_string(true)
             << ": atof.substr(40,8)=" << atof.substr(40,8).to_string(true)
             << ": overall sum=" << atof3.to_string(true);
  CHECK(atof3 == atof);
}

void StringPrefixTester::PrefixTest(void) {
  StringPrefix abcbef = StringPrefix{"abcbef"};
  StringPrefix abcqef = StringPrefix{"abcqef"};

  StringPrefix pref = abcbef.prefix(abcqef);
  DLOG(INFO) << "abcbef=" << abcbef.to_string(true)
             << ": abcqef=" << abcqef.to_string(true)
             << ": abcbef.prefix(abcqef)=" << pref.to_string(true);
  // "b".prefix("q").size == 3
  CHECK_EQ(pref.size(), 27);
  StringPrefix empty = StringPrefix{""};
  DLOG(INFO) << "abcbef=" << abcbef.to_string(true)
             << ": empty=" << empty.to_string(true)
             << ": abcbef.prefix(empty)=" 
             << abcbef.prefix(empty).to_string(true);
  CHECK_EQ(empty.prefix(abcbef).size(), 0);
  CHECK_EQ(abcbef.prefix(empty).size(), 0);
  StringPrefix b = StringPrefix{"b"};
  StringPrefix pref2 = abcbef.prefix(b);
  DLOG(INFO) << "abcbef=" << abcbef.to_string(true)
             << ": b=" << b.to_string(true)
             << ": abcbef.prefix(b)=" 
             << pref2.to_string(true);
  CHECK_EQ(pref2.size(), 6);
  
  StringPrefix ab = "Arijit Baba Nam";
  StringPrefix ad = "Aditya Me";
  StringPrefix pref3 = ab.prefix(ad);
  DLOG(INFO) << "ab=" << ab.to_string(true)
             << ": ad=" << ad.to_string(true)
             << ": ab.prefix(ad)=" 
             << pref3.to_string(true);
  
  CHECK_EQ(ab.prefix(ad).size(), 11);
  CHECK_EQ(ad.prefix(ab).size(), 11);
}

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  StringPrefixTester spt;

  spt.SanityTest();
  spt.PlusSubstrTest();
  spt.PrefixTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
