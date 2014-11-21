/* demo_uts_namespaces.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Demonstrate the operation of UTS namespaces.
*/
/* ns_exec.c 

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Join a namespace and execute a command in the namespace
*/
/* unshare.c 

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   A simple implementation of the unshare(1) command: unshare
   namespaces and execute a command.
*/

#ifdef _GNU_SOURCE

// C++ Standard Headers
#include <fstream>          // ifstream, ofstream
#include <functional>       // std::function<>
#include <iostream>        
#include <sstream>
// C Standard Headers
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/utsname.h>
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

class TestNS {
 public:
  TestNS(function<void(void)> test_fn): cargs_{test_fn, {0,0}} {}
  int RunTest(void) {
    if (pipe(cargs_.pipe_fd) < 0)
      ProcError("pipe");

    pid_t cpid = CloneNS();
    // Give root capability to the new NameSpace
    UpdateUidMap(cpid, 0);
    
    // We are write end: close the read end
    close(cargs_.pipe_fd[READ]);
    // Close the write end of the pipe, to signal to the child that we
    // have updated the UID and GID maps
    close(cargs_.pipe_fd[WRITE]);
    
    ReapChild(cpid);
    
    return 0;
  }

  static int ReapChild(pid_t cpid) {
    pid_t pid = getpid();
    pid_t ppid = getppid();
    
    LOG(INFO) << "Parent PID " << pid << " waiting for child PID " << cpid
              << " termination - GrandParent PID " << ppid;
    
    // Wait for child
    if (waitpid(cpid, NULL, 0) < 0)
      ProcError("waitpid");

    LOG(INFO) << "Parent PID " << pid << " reaped child PID " 
              << cpid << " after termination - GrandParent PID " << ppid;
    
    return 0;
  }

  static void ProcError(const char *str) {
    LOG(ERROR) << str << ": " << strerror(errno);
    throw SystemError(str);
  }

  static char* new_stack(void) {
    char *p = new char[STACK_SIZE];
    return (p + STACK_SIZE);
  }

 private:
  static constexpr size_t READ=0; 
  static constexpr size_t WRITE=1;
  static constexpr size_t NUM_ENDS=WRITE+1;

  struct SystemError {
    SystemError(const char *errstr) : str(errstr){}
    const char *str;
  };
  struct ChildArgs {
    function<void(void)> Test_Fn;
    int pipe_fd[NUM_ENDS];
  };
  ChildArgs cargs_;

  static constexpr size_t STACK_SIZE = 1024*1024;

  // * CLONE_NEWUSER: Creates a new UID NS. We can have root privileges in the
  //   new UID namespace (UID == 0) and execute tests that would otherwise
  //   require root privileges
  // * SIGCHLD: The low byte of flags contains the number of the termination 
  //   signal sent to the parent when the child dies.
  static constexpr int CLONE_FLAGS = CLONE_NEWUSER | SIGCHLD;

  static int SetUpRootNSFunc(void *arg) {
    ChildArgs *cargs_p = static_cast<ChildArgs *>(arg);
    
    // We are read end. Close write end
    close(cargs_p->pipe_fd[WRITE]);
    
    pid_t pid_my = getpid();
    
    LOG(INFO) << "PID " << pid_my
              << " reading from pipe as signal to continue test...";
    
    char ch;
    // should read EOF as signal that uid map is complete
    if (read(cargs_p->pipe_fd[READ], &ch, 1) != 0)
      ProcError("read-pid");

    close(cargs_p->pipe_fd[READ]);
    uid_t uid_my = geteuid();
    
    LOG(INFO) << "PID " << pid_my << " eUID " << uid_my << " running test...";
    cargs_p->Test_Fn();
    LOG(INFO) << "PID " << pid_my << " eUID " << uid_my << " completed test...";
    
    return 0;
  }

  // Clone NS and return PID of the child in the parent NS 
  pid_t CloneNS() {
    // Create a child that has its own PID namespace;
    // the child commences execution in childFunc()
    pid_t child_pid = clone(SetUpRootNSFunc, 
                            // Points to start of downwardly growing stack
                            new_stack(),
                            CLONE_FLAGS, &cargs_);
    if (child_pid < 0)
      ProcError("clone");
    
    LOG(INFO) << "PID: Parent " << getpid() << ": Clone Child " << child_pid 
              << ": GrandParent " << getppid();
    
    return child_pid;
  }

  int UpdateUidMap(pid_t cpid, uid_t uid) {
    ostringstream oss;
    oss << "/proc/" << cpid << "/uid_map";
    
    pid_t pid = getpid();
    uid_t uid_parent = geteuid();
    
    LOG(INFO) << "Parent " << pid << " uid_parent " << uid_parent
              << " writing for uid " << uid << " uid_map file " << oss.str();
    
    // Write file: the file should have only one line. line format
    // uid uid_parent 1
    ofstream ofs;
    ofs.open(oss.str(), std::ofstream::out);
    CHECK(ofs);
    ofs << uid << " " << uid_parent << " " << 1;
    ofs.close();
    
    LOG(INFO) << "Parent " << pid << " uid_parent " << uid_parent
              << " wrote uid_map file " << oss.str() << " line: "
              << uid << " " << uid_parent << " " << 1;
    
    return 0;
  }
};

class UtsNSTester {
 public:
  // TEST1:
  // 1. BACKGROUND: Child created via clone() CLONE_NEWUTS param.
  // 2. Set hostname of child to another name ns_name
  // 3. Validate the new hostname of child.
  int Test1UTSChildNS(char *ns_name, pid_t *cpid_p) {
    child_ns_name_ = ns_name;
    my_pid_ = getpid();
    // Create a child that has its own UTS namespace;
    // the child commences execution in childFunc()
    *cpid_p = clone(UTSChildNSFunc, 
                    // Points to start of downwardly growing stack
                    TestNS::new_stack(),   
                    CLONE_FLAGS, 
                    const_cast<void *>(static_cast<const void *>(child_ns_name_)));
    if (*cpid_p < 0)
      TestNS::ProcError("clone");

    LOG(INFO) << "PID of parent " << my_pid_
              << ": child created by clone() " << *cpid_p;

    // Display the hostname in parent's UTS namespace. 
    // This will be different from the hostname in child's UTS namespace.
    if (uname(&uts) < 0)
      TestNS::ProcError("uname");
    
    LOG(INFO) << "PID my_pid " << "uts.nodename in parent: " << uts.nodename;    
    
    return 0;
  }

  // TEST2: 
  // 1. BACKGROUND: Child created via clone() CLONE_NEWUTS param.
  // 2. Set UTS NS of the Parent with setns of Child NS
  // 3. Validate the hostname of parent is same as that of child.
  int Test2UTSSetNS(pid_t cpid) {
    int    fd;
    ostringstream oss;
    
    oss << "/proc/" << cpid << "/ns/uts";
    LOG(INFO) << "PID my_pid " << my_pid_ 
              << " parent opening file " << oss.str();    
    // Get descriptor for namespace
    fd = open(oss.str().c_str(), O_RDONLY);   
    if (fd < 0)
      TestNS::ProcError("open");
    
    // Join that namespace
    if (setns(fd, CLONE_NEWUTS) < -1)         
      TestNS::ProcError("setns");

    close(fd);

    // Give child time to change its hostname
    usleep(50000);

    // Validate that gethostname returns the same string as child_ns_name_
    if (uname(&uts) < 0)
      TestNS::ProcError("uname");

    LOG(INFO) << "PID my_pid " << my_pid_ 
              << " post setns(): uts.nodename in parent: " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, child_ns_name_), 0);

    return 0;
  }

  // TEST3: 
  // 1. BACKGROUND: Child created via clone() CLONE_NEWUTS param
  //                Parent also in Child UTS_NS via setns call.
  // 2. Set UTS NS of the Parent with unshare call with new_ns_name NS
  // 3. Validate the hostname of parent is same as new_ns_name.
  int Test3UTSUnshareNS(char *new_ns_name) {
    // Retrieve and display hostname
    if (uname(&uts) < 0)
      TestNS::ProcError("uname");

    LOG(INFO) << "PID " << my_pid_ << " parent pre-unshare hostname() " << uts.nodename;

    if (unshare(CLONE_NEWUTS) < 0)
      TestNS::ProcError("unshare");

    if (sethostname(new_ns_name, strlen(new_ns_name)) < 0)
      TestNS::ProcError("sethostname");
    
    // Retrieve and display hostname
    if (uname(&uts) < 0)
      TestNS::ProcError("uname");

    LOG(INFO) << "PID " << my_pid_ << " parent post-unshare hostname() " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, new_ns_name), 0);

    return 0;
  }
  
 private:
  // * CLONE_NEWUTS: Creates a new UTS NS. Impacts hostname and domain name
  // * SIGCHLD: The low byte of flags contains the number of the termination 
  //   signal sent to the parent when the child dies.
  static constexpr int CLONE_FLAGS = CLONE_NEWUTS | SIGCHLD;

  char *child_ns_name_;
  pid_t my_pid_;
  struct utsname uts;

  static int UTSChildNSFunc(void *arg)
  {
    struct utsname uts;
    pid_t  child_pid = getpid();
  
    // Retrieve and display hostname
    if (uname(&uts) < 0) 
      TestNS::ProcError("uname");

    LOG(INFO) << "PID " << child_pid << " child pre-sethostname(): " << uts.nodename;

    // Change hostname in UTS namespace of child
    char *child_ns_name = static_cast<char *>(arg);
    
    if (sethostname(child_ns_name, strlen(child_ns_name)) < 0)
      TestNS::ProcError("sethostname");
    
    // Retrieve and display hostname
    if (uname(&uts) < 0)
      TestNS::ProcError("uname");
    
    LOG(INFO) << "PID " << child_pid << " child post-sethostname(): " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, child_ns_name), 0);
    
    // Keep the namespace open for a while, by sleeping.
    // This allows some experimentation-- 
    // for example, another process might join the namespace.
    usleep(100000);

    // Terminates child
    LOG(INFO) << "PID " << child_pid << " child has terminated";
    
    return 0;           
  }
};

int
main(int argc, char *argv[])
{
  Init::InitEnv(&argc, &argv);

  TestNS test(
      []{
        UtsNSTester utns;
        pid_t cpid{0};
        char ns1[] = "Ory1";
        utns.Test1UTSChildNS(ns1, &cpid);
        utns.Test2UTSSetNS(cpid);
        char ns2[] = "<Ory2";
        utns.Test3UTSUnshareNS(ns2);
        TestNS::ReapChild(cpid);
      });

  test.RunTest();

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

#endif // _GNU_SOURCE
