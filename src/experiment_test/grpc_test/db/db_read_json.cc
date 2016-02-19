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

/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Standard C++ Headers
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "experiment_test/grpc_test/db/db_read_json.h"

using namespace std;
using asarcar::routeguide::Feature;

namespace asarcar { namespace routeguide {
//-----------------------------------------------------------------------------

const string DbReadJSON::kLocation  = "\"location\":";
const string DbReadJSON::kLatitude  = "\"latitude\":";
const string DbReadJSON::kLongitude = "\"longitude\":";
const string DbReadJSON::kName      = "\"name\":";

DbReadJSON::DbReadJSON(string&& db_str) {
  // Remove all spaces.
  auto it_last = std::remove_if(db_str.begin(), db_str.end(), 
                                [](char ch){return isspace(ch);});
  db_str.erase(it_last, db_str.end());
  db_ = move(db_str);
  
  return;
}

void DbReadJSON::Parse(vector<Feature>* flist_p) {
  current_ = 0;
  failed_  = false;
  flist_p->clear();

  if (!Match("[")) {
    LOG(ERROR) << "Error parsing: opening '[' missing";
    SetFailedAndReturnFalse();
    return;
  }
  if (db_[current_] == ']' && current_ == db_.size()-1)
    return;

  while (!Finished()) {
    flist_p->push_back(Feature());
    if (!TryParseOne(&flist_p->back())) {
      LOG(ERROR) << "Error parsing the db file";
      flist_p->clear();
      return;
    }
  }
}

string DbReadJSON::ReadJSONFile(const string& db_file_name) {
  ifstream db_file(db_file_name);
  if (!db_file.is_open()) {
    LOG(ERROR) << "Failed to open " << db_file_name << endl;
    return "";
  }
  stringstream db;
  db << db_file.rdbuf();
  return db.str();
}

bool DbReadJSON::TryParseOne(Feature* flist_p) {
  if (failed_ || Finished() || !Match("{")) {
    return SetFailedAndReturnFalse();
  }
  if (!Match(kLocation) || !Match("{") || !Match(kLatitude)) {
    return SetFailedAndReturnFalse();
  }
  long temp = 0;
  ReadLong(&temp);
  flist_p->mutable_location()->set_latitude(temp);
  if (!Match(",") || !Match(kLongitude)) {
    return SetFailedAndReturnFalse();
  }
  ReadLong(&temp);
  flist_p->mutable_location()->set_longitude(temp);
  if (!Match("},") || !Match(kName) || !Match("\"")) {
    return SetFailedAndReturnFalse();
  }
  size_t name_start = current_;
  while (current_ != db_.size() && db_[current_++] != '"');
  if (current_ == db_.size()) {
    return SetFailedAndReturnFalse();
  }
  flist_p->set_name(db_.substr(name_start, current_-name_start-1));
  if (!Match("},")) {
    if (db_[current_ - 1] == ']' && current_ == db_.size()) {
      return true;
    }
    return SetFailedAndReturnFalse();
  }
  return true;
}

void DbReadJSON::ReadLong(long* l) {
  size_t start = current_;
  while (current_ != db_.size() && 
         db_[current_] != ',' && db_[current_] != '}') {
    current_++;
  }
  // It will throw an exception if fails.
  *l = stol(db_.substr(start, current_ - start));
}

bool DbReadJSON::Match(const string& prefix) {
  bool eq = db_.substr(current_, prefix.size()) == prefix;
  current_ += prefix.size();
  return eq;
}

//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace routeguide

