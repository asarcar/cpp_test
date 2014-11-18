/* multi_pidns.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Create a series of child processes in nested PID namespaces.
*/
#ifdef _GNU_SOURCE

// C++ Standard Headers
#include <iostream>        
#include <sstream>
// C Standard Headers
#include <climits>
#include <cstring>
#include <cstdlib>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// Google Headers
#include <gflags/gflags.h>  // Parse command line args and flags
#include <glog/logging.h>   // Daemon Log function
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/init.h"

using namespace asarcar;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

class SystemError {
 public:
  SystemError(const char *str) : str_(str){}
 private:
  const char *str_;
};

class PidNSTester {
 public:
  explicit PidNSTester(int level) : level_{level} {
    DCHECK(level_ >= 0);
    LOG_IF(FATAL, level_ <= 0) << "Only works with +ve level";
  }

  // Called only from the top level
  int TestPidNS(void) {
    pid_t child_pid = ClonePidNS(level_);
    // PID 1 is reserved for first process (init) in every NS
    CHECK_GT(child_pid, 1);
    WaitChildTerminate(child_pid);
    return 0;
  }

  static char* new_stack(void) {
    char *p = new char[STACK_SIZE];
    return (p + STACK_SIZE);
  }

 private:
  static constexpr size_t STACK_SIZE = 1024*1024;
  int level_;

  // PID Clone NS and return PID of the child in the parent NS 
  static pid_t ClonePidNS(int level) {
    DCHECK(level > 0);

    // Create a child that has its own UTS namespace;
    // the child commences execution in childFunc()
    pid_t child_pid = clone(PidNSFunc, 
                            // Points to start of downwardly growing stack
                            new_stack(),
                            CLONE_NEWPID | SIGCHLD, 
                            reinterpret_cast<void *>(level - 1));
    if (child_pid < 0) {
      ProcError("clone");
    }

    LOG(INFO) << "Level " << level 
              << ": PID: Parent " << getpid() << ": Clone Child " << child_pid 
              << ": GrandParent " << getppid();

    return child_pid;
  }
  
  static void ProcError(const char *str) {
    LOG(ERROR) << str << ": " << strerror(errno);
    throw SystemError(str);
  }

  static int MountProc(int level) {
    ostringstream oss;
    oss << "/proc" << level;

    // Create directory for mount point
    mkdir(oss.str().c_str(), 0555); 

    if (mount("proc", oss.str().c_str(), "proc", 0, NULL) < 0)
      ProcError("mount");

    LOG(INFO) << "PID " << getpid() 
              << ": Mounting procfs and create directory at " << oss.str();

    return 0;
  }

  static int UnMountProc(int level) {
    ostringstream oss;
    oss << "/proc" << level;

    if (umount(oss.str().c_str()) < 0)
      ProcError("mount");

    // Remove directory from mount point
    rmdir(oss.str().c_str()); 

    LOG(INFO) << "PID " << getpid() 
              << ": Removing Direcotry and UnMounting procfs from " << oss.str();

    return 0;
  }

  static int WaitChildTerminate(pid_t child_pid) {
    pid_t pid = getpid();
    pid_t ppid = getppid();

    LOG(INFO) << "Parent waiting for child termination"
              << ": Child PID " << child_pid
              << ": Parent PID " << pid 
              << ": GrandParent PID " << ppid;

    // Wait for child
    if (waitpid(child_pid, NULL, 0) < 0) {
      ProcError("waitpid");
    }

    LOG(INFO) << "Parent terminating after child termination"
              << ": Child PID " << child_pid
              << ": Parent PID " << pid 
              << ": GrandParent PID " << ppid;

    return 0;
  }

  // Called in the context of private NS
  // Recursively create a series of child process in nested PID namespaces.
  // 'arg' is an integer that counts down to 0 during the recursion.
  // When the counter reaches 0, recursion stops and the tail child
  // executes the sleep(100) call.
  static int PidNSFunc(void *arg) {
    pid_t ppid = getppid();
    CHECK_EQ(ppid, 0);
    pid_t pid = getpid();
    CHECK_EQ(pid, 1);

    intptr_t tmp = reinterpret_cast<intptr_t>(arg);
    int level = tmp;

    // mount a procfs for the current PID namespace
    MountProc(level);

    // Recursion termination condition: final level of recursion
    if (level <= 0) {
      LOG(INFO) << "Lowest level process PID: pid " << pid 
                << ": ppid " << ppid << " terminating...";

      // Sleep while debugging inorder to test PID NS commands
      // sudo readlink /proc<n>/<pid>/ns/pid: get PID NS link from procfs
      // cat /proc<n>/pid/status | egrep '^(Name|PP*id)' : get PID from procfs
      // sleep(100);

      // UnMount procfs for the current PID namespace
      UnMountProc(level);
      
      return 0;
    }
       
    // Recursively invoke childFunc() to create another child in a nested PID namespace
    pid_t child_pid = ClonePidNS(level);
    CHECK_EQ(child_pid, 2);

    WaitChildTerminate(child_pid);

    // UnMount procfs for the current PID namespace
    UnMountProc(level);

    return 0;
  }
};

int
main(int argc, char *argv[])
{
  Init::InitEnv(&argc, &argv);

  PidNSTester pid_ns(3);
  pid_ns.TestPidNS();
  
  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

#endif // _GNU_SOURCE
