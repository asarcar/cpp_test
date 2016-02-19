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

//! @file   init.cc
//! @brief  Implementation: init environment, enable logging, parse flags, etc.
//! @author Arijit Sarcar <sarcar_a@yahoo.com>

// C++ Standard Headers
#include <iostream>         // std::cout
// C Standard Headers
// Google Headers
#include <gflags/gflags.h>  // Parse command line args and flags
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"
#include "utils/basic/fassert.h"

DECLARE_bool(logtostderr);
DECLARE_string(log_dir);

namespace asarcar { 

void Init::InitEnv(int *argc_p, char **argv_p[]) {
  FASSERT(argc_p != nullptr);
  FASSERT(argv_p != nullptr);

  google::ParseCommandLineFlags(argc_p, argv_p, true);
  if (FLAGS_log_dir.empty()) {
    // Every test *should* be invoked with an appropriate environment variable 
    // for the output directory where all logs and output goes
    // if log_dir is not set just direct all logs to the TEST OUTPUT directory
    const char *test_op_dir = getenv("TEST_OUTPUT_DIR");
    if (test_op_dir != nullptr)
      FLAGS_log_dir = test_op_dir;
    // else FLAGS_logtostderr = true; // Do not dump log on console unless enabled
  }

  google::InitGoogleLogging((*argv_p)[0]);
  google::InstallFailureSignalHandler();

  DLOG(INFO) << "Program " << (*argv_p)[0] << " initialized"
             << ": logtostderr="<< std::boolalpha << "\"" 
             << FLAGS_logtostderr << "\""
             << ": log_dir=" << "\"" << FLAGS_log_dir << "\""
             << ": HEAPCHECK=" << "\"" << GetEnvStr("HEAPCHECK") << "\""
             << ": HEAPCHECK_DUMP_DIRECTORY=" 
             << "\"" << GetEnvStr("HEAPCHECK_DUMP_DIRECTORY") << "\""
             << ": HEAPPROFILE=" << "\"" << GetEnvStr("HEAPPROFILE") << "\""
             << ": CPUPROFILE=" << "\"" << GetEnvStr("CPUPROFILE") << "\"";

  return;
}

std::string Init::GetEnvStr(const char* str)
{
  const char *env = getenv(str);
  if (env == nullptr) 
    return std::string("");

  return std::string(env);
}

} // namespace asarcar


