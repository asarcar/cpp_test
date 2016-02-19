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
#include "experiment_test/grpc_test/db/db_read_json.h"
#include "experiment_test/grpc_test/protos/route_guide.pb.h"
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"

using namespace asarcar;
using namespace asarcar::routeguide;
using namespace std;

// Declarations
DECLARE_bool(auto_test);

class DbReadJSONTest {
 public:
  DbReadJSONTest()  = default;
  ~DbReadJSONTest() = default;
  void   TestBasic(void);
  void   TestMany(void);
  void   TestError(void);
 private:
};

void DbReadJSONTest::TestBasic() {
  vector<Feature> flist;

  DbReadJSON dbrj{"[]"};
  dbrj.Parse(&flist);
  CHECK_EQ(flist.size(), 0);

  DbReadJSON dbrj2{
    "[\
       {\
         \"location\":\
         {\
           \"latitude\": 407838351,\
           \"longitude\": -746143763\
         },\
         \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
       }\
     ]"
        };
  dbrj2.Parse(&flist);
  CHECK_EQ(flist.size(), 1);
  CHECK_EQ(flist.at(0).location().latitude(), 407838351);
  CHECK_EQ(flist.at(0).location().longitude(), -746143763);
  CHECK_STREQ(flist.at(0).name().c_str(), "PatriotsPath,Mendham,NJ07945,USA");
}

void DbReadJSONTest::TestMany() {
  vector<Feature> flist;
  DbReadJSON dbrj{
    "[\
       {\
         \"location\":\
         {\
           \"latitude\": 407838351,\
           \"longitude\": -746143763\
         },\
         \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
       },\
       {\
         \"location\":\
         {\
           \"latitude\": 507838351,\
           \"longitude\": -846143763\
         },\
         \"name\": \"\"\
       }\
     ]"
        };
  dbrj.Parse(&flist);
  CHECK_EQ(flist.size(), 2);
  CHECK_EQ(flist.at(1).location().latitude(), 507838351);
  CHECK_EQ(flist.at(1).location().longitude(), -846143763);
  CHECK_STREQ(flist.at(1).name().c_str(), "");
}

void DbReadJSONTest::TestError() {
  vector<Feature> flist;
  DbReadJSON dbrj{
    "[\
       {\
         \"location\":\
         {\
           \"latitude\": 407838351,\
           \"longitude\": -746143763\
         },\
         \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
       }"
        };
  dbrj.Parse(&flist);
  CHECK_EQ(flist.size(), 0);

  DbReadJSON dbrj2{
    "[\
       {\
         \"location\":\
         {\
           \"latitude\": 407838351,\
           \"longitude\": -746143763\
         }\
         \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
       }\
     ]"
        };
  dbrj2.Parse(&flist);
  CHECK_EQ(flist.size(), 0);

  DbReadJSON dbrj3{
    "[\
       {\
         \"location\":\
         {\
           \"latitude\": 407838351,\
         },\
         \"name\": \"Patriots Path, Mendham, NJ 07945, USA\"\
       }\
     ]"
        };
  dbrj3.Parse(&flist);
  CHECK_EQ(flist.size(), 0);
}

int main(int argc, char *argv[]) {
  Init::InitEnv(&argc, &argv);

  DbReadJSONTest dbRdJson{};
  dbRdJson.TestBasic();
  dbRdJson.TestMany();
  // dbRdJson.TestError();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
