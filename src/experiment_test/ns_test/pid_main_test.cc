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
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
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

  // TEST1: New PID NS created with CLONE_NEWPID
  // 1. Spawn children, grandchild, upto N levels as passed to the function
  // 2. Each child at each level is spawned in a new PID NS.
  // 3. Verify that the "cloned" process in each new NS has PID 1, 
  //    parent has PID 0 (as it was spawned in another NS that is not visible),
  //    and child of cloned process has PID 2 in parent NS and 1 in its NS.
  int Test1PidNS(int level) {
    pid_t child_pid = ClonePidNS(level);
    // PID 1 is reserved for first process (init) in every NS
    CHECK_GT(child_pid, 1);
    ReapChild(child_pid);
    return 0;
  }

  // TEST2 (Assume test spawned is PID Base in Namespace BASE)
  // 2.1: ClonePidNS: Base clones child C1 in new PID_NS (CHILD_NS)
  //      Validate C1-PID 1 (already tested in TEST1). 
  // 2.2: Fork C2. Validate C2-PID is 2 & C2-PPID is 1.
  // 2.3: C2 forks C3. Validate C3-PID is 3 & C3-PPID is 2. 
  // 2.4: C2 exits. C3 orphaned. Validate C1 is reaps C2. Validate C3 reparented to C1.
  // 2.5: C3 exits. Validate C1 reaps C3.  
  // 2.6: C1 exits.
  int Test2PidNS(void) {
    // 2.1: ClonePidNS: Clone Child C1
    // 2.5: C1 reaps C3
    // 2.6: C1 exits.
    pid_t c1pid = Test2_Run();

    // 2.6: C1 exits: Validate it is reaped by Base.
    ReapChild(c1pid);

    return 0;
  }

  static char* new_stack(void) {
    char *p = new char[STACK_SIZE];
    return (p + STACK_SIZE);
  }

 private:
  static constexpr size_t READ=0; 
  static constexpr size_t WRITE=1;

  static constexpr size_t STACK_SIZE = 1024*1024;

  // PID Clone NS and return PID of the child in the parent NS 
  static pid_t ClonePidNS(int level) {
    DCHECK(level > 0);

    // Create a child that has its own PID namespace;
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

  static int ReapChild(pid_t child_pid) {
    pid_t pid = getpid();
    pid_t ppid = getppid();

    LOG(INFO) << "Parent PID " << pid << " waiting for child PID " << child_pid
              << " termination - GrandParent PID " << ppid;

    // Wait for child
    if (waitpid(child_pid, NULL, 0) < 0) {
      ProcError("waitpid");
    }

    LOG(INFO) << "Parent PID " << pid << " reaped child PID " 
              << child_pid << " after termination - GrandParent PID " << ppid;

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

    ReapChild(child_pid);

    // UnMount procfs for the current PID namespace
    UnMountProc(level);

    return 0;
  }

  // 2.1: ClonePidNS: Clone Child C1
  pid_t Test2_Run(void) {
    pid_t pid = getpid();
    
    // Create a child that has its own PID namespace;
    // the child commences execution in Test2_1_Func()
    pid_t c1pid = clone(Test2_Func, 
                        // Points to start of downwardly growing stack
                        new_stack(), CLONE_NEWPID | SIGCHLD, NULL);
    LOG(INFO) << "PID " << pid << " Cloned C1-PID "<< c1pid << " in NEW_PID NS";
    CHECK_GT(c1pid, pid);

    return c1pid;
  }

  // 2.4: Validate C1 reaps C2.
  // 2.5: C1 reaps C3
  // 2.6: C1 exit.
  static int Test2_Func(void *arg) {
    pid_t pid = getpid();
    pid_t ppid = getppid();

    LOG(INFO) << "C1-PID " << pid << " pPID " << ppid << "(new PID_NS)";

    CHECK_EQ(pid, 1);
    CHECK_EQ(ppid, 0);

    Test2_Remaining();
    
    ReapChild(2); // interrupted by SIGCHLD signal for C2
    ReapChild(3); // interrupted by SIGCHLD signal for C3

    return 0;
  }

  // 2.2: Fork C2. Validate C2-PID is 2 & C2-PPID is 1.
  // 2.3: C2 forks C3. Validate C3-PID is 3 & C3-PPID is 2. 
  // 2.4: C2 exits. C3 orphaned. Validate C1 is reaps C2. Validate C3 reparented to C1.
  // 2.5: C3 exits. Validate C1 reaps C3.  
  // 2.6: C1 exits.
  static int Test2_Remaining(void) {
    pid_t pid = getpid();
    CHECK_EQ(pid, 1);
    pid_t ppid = getppid();
  
    LOG(INFO) << "C1-PID " << pid << " pPID " << ppid << " forking C2";
    pid_t c2pid = fork();

    if (c2pid < 0) {
      ProcError("fork-C2");
    }

    // Parent (PID-Base) Context: Return c2pid
    if (c2pid > 0) {
      CHECK_EQ(c2pid, 2);
      LOG(INFO) << "C1-PID " << pid << " pPID " << ppid 
                << " forked C2-PID " << c2pid;
      return c2pid;
    }
    // Child C2 Context (i.e. c2pid == 0): Fork child C3 and exit with SUCCESS
    pid = getpid();
    ppid = getppid();
    LOG(INFO) << "C2-PID " << pid << " pPID " << ppid << " now forking C3";
    pid_t c3pid = fork();

    if (c3pid < 0) {
      ProcError("fork-C3");
    }

    // Parent C2 context: 
    if (c3pid > 0) {
      CHECK_EQ(c3pid, 3);
      LOG(INFO) << "C2-PID " << pid << " orphaning C3-PID " << c3pid << " exiting...";
      exit(EXIT_SUCCESS); // TODO: C++ throw exception to unwind stack cleanly
    }

    // Child (C3) Context i.e. c3pid == 0 
    pid = getpid();
    ppid = getppid();
    LOG(INFO) << "C3-PID " << pid << " pPID " << ppid 
              << " waiting for orphaned child to reparent to C1";

    // loop will terminate once child is orphaned. 
    // C3 should get re-parented to C1 after it is orphaned by C2
    while (ppid != 1) {
      usleep(10000);
      ppid = getppid();
    } 

    LOG(INFO) << "C3-PID " << pid << " pPID " << ppid << " exiting now...";
    exit(EXIT_SUCCESS); // TODO: C++ throw exception to unwind stack cleanly

    return 0;
  }
};

int
main(int argc, char *argv[])
{
  Init::InitEnv(&argc, &argv);

  PidNSTester pid_ns;
  pid_ns.Test1PidNS(3);
  pid_ns.Test2PidNS();
  
  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

#endif // _GNU_SOURCE
