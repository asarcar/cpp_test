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
#include <iostream>
#include <string>        
#include <unordered_map>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
#include <leveldb/db.h>
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

using namespace std;
using namespace leveldb;
using namespace asarcar;

class LevDbOpsTester {
 private:
 public:
  LevDbOpsTester() = default;
  ~LevDbOpsTester() = default;
  void Run(void) {
    InitFile();
    ReadTest();
    return;
  }
 private:
  unordered_map<string,string> word_map_ = {
    {"Hello", "World"}, {"Raja", "Maharaja"}
  };
 private:
  void InitFile(void) {
    DB *db_p;
    Options options;

    options.create_if_missing = true;

    Status status = DB::Open(options, "./leveldb_dir", &db_p);
    assert(status.ok());
  
    WriteOptions woptions;

    LOG(INFO) << "Initializing Level DataBase: Directory";
    for (auto &w : word_map_) {
      LOG(INFO) << "Add: <" << w.first << ", " << w.second << ">"; 
      db_p->Put(woptions, w.first, w.second);
    }
    LOG(INFO) << "Completed Initializing Level DataBase";

    delete db_p;
  
    return;
  }

  void ReadTest(void) {
    DB *db_p;
    Options options;

    Status status = DB::Open(options, "./leveldb_dir", &db_p);
    assert(status.ok());

    ReadOptions roptions;
    
    Iterator *it_p = db_p->NewIterator(roptions);

    LOG(INFO) << "Reading Level DataBase";
    for(it_p->SeekToFirst(); it_p->Valid(); it_p->Next()) {
      string first  = it_p->key().ToString();
      string second = it_p->value().ToString();
      LOG(INFO) << "Retrieve: <" << first << ", " << second << ">"; 
      auto it = word_map_.find(first);
      FASSERT(it != word_map_.end());
      CHECK_EQ(it->second, second);
    }
    LOG(INFO) << "Completed Reading Level DataBase";
  
    delete it_p;
    delete db_p;

    return;
  }
};

// Flag Declarations
DECLARE_bool(auto_test);

int main(int argc, char** argv) {
  Init::InitEnv(&argc, &argv);

  LOG(INFO) << argv[0] << " Executing Test";
  LevDbOpsTester ldbot;
  ldbot.Run();
  LOG(INFO) << argv[0] << " Test Passed";

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");
