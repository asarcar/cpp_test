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

//! @file   db_read_json.h
//! @brief  Parses and reads a flat file of JSON records
//! @detail File Format: 
//!         [{"location": {"latitude": latval, "longitude": longval}, 
//!           "name": "str"},
//!          ...]
//!         Example:
//!         {"location":{"latitude": 407838351,"longitude": -746143763},
//!          "name": "Patriots Path, Mendham, NJ 07945, USA"}
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

#ifndef _EXPERIMENT_TEST_DB_READ_JSON_H_
#define _EXPERIMENT_TEST_DB_READ_JSON_H_

// Standard C++ Headers
#include <string>
#include <vector>
// Standard C Headers
// Google Headers
// Local Headers
#include "experiment_test/grpc_test/protos/route_guide.pb.h"

namespace asarcar { namespace routeguide { 
//-----------------------------------------------------------------------------
// A simple parse read for the json db file. Requires the db file to have the
// exact form of [{"location": { "latitude": 123, "longitude": 456}, "name":
// "the name can be empty" }, { ... } ... The spaces will be stripped.
class DbReadJSON {
 public:
  explicit DbReadJSON(std::string&& db_str);
  // Prevent bad usage: copy and assignment
  DbReadJSON(const DbReadJSON&)             = delete;
  DbReadJSON& operator =(const DbReadJSON&) = delete;
  DbReadJSON(DbReadJSON&&)                  = delete;
  DbReadJSON& operator =(DbReadJSON&&)      = delete;

  void Parse(std::vector<Feature>* flist_p);

  static std::string ReadJSONFile(const std::string& db_file_name);
 private:
  const static std::string kLocation;
  const static std::string kLatitude;
  const static std::string kLongitude;
  const static std::string kName;
  size_t current_                           = 0;
  bool failed_                              = false;
  std::string db_                           = "";

  bool TryParseOne(Feature* feature);
  bool Match(const std::string& prefix);
  void ReadLong(long* l);
  inline bool SetFailedAndReturnFalse(void) {failed_ = false; return false;}
  inline bool Finished() {return current_ >= db_.size();}
};
//-----------------------------------------------------------------------------
} } // namespace asarcar { namespace routeguide { 

#endif // _EXPERIMENT_TEST_DB_READ_JSON_H_

