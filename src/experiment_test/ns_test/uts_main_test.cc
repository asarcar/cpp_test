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

class SystemError {
 public:
  SystemError(const char *str) : str_(str){}
 private:
  const char *str_;
};

class UtsNSTester {
 public:
  static constexpr size_t STACK_SIZE = 1024*1024;
  char child_stack[STACK_SIZE];
  explicit UtsNSTester(const char *child_ns_name): child_ns_name_(child_ns_name) {
    my_pid_ = getpid();
    // Create a child that has its own UTS namespace;
    // the child commences execution in childFunc()
    child_pid_ = clone(TestUTSChildNS, 
                       // Points to start of downwardly growing stack
                       child_stack + STACK_SIZE,   
                       CLONE_NEWUTS | SIGCHLD, 
                       const_cast<void *>(static_cast<const void *>(child_ns_name_)));
    if (child_pid_ < 0) {
      ProcError("clone");
    }

    return;
  }

  int DumpUTSParentNS(void) {
    LOG(INFO) << "PID of parent " << my_pid_
              << ": child created by clone() " << child_pid_;

    // Display the hostname in parent's UTS namespace. 
    // This will be different from the hostname in child's UTS namespace.
    if (uname(&uts) < 0) {
      ProcError("uname");
    }
    
    LOG(INFO) << "PID my_pid " << "uts.nodename in parent: " << uts.nodename;    
    
    return 0;
  }

  int TestUTSSetNS(void) {
    int    fd;
    ostringstream oss;
    
    oss << "/proc/" << child_pid_ << "/ns/uts";
    LOG(INFO) << "PID my_pid " << my_pid_ 
              << " parent opening file " << oss.str();    
    // Get descriptor for namespace
    fd = open(oss.str().c_str(), O_RDONLY);   
    if (fd < 0)
      ProcError("open");
    
    // Join that namespace
    if (setns(fd, CLONE_NEWUTS) < -1)         
      ProcError("setns");

    close(fd);

    // Give child time to change its hostname
    sleep(1);

    // Validate that gethostname returns the same string as child_ns_name_
    if (uname(&uts) < 0) {
      ProcError("uname");
    }
    LOG(INFO) << "PID my_pid " << my_pid_ 
              << " post setns(): uts.nodename in parent: " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, child_ns_name_), 0);

    return 0;
  }

  int TestUTSUnshareNS(void) {
    // Retrieve and display hostname
    if (uname(&uts) < 0) {
      ProcError("uname");
    }

    LOG(INFO) << "PID " << my_pid_ << " parent pre-unshare hostname() " << uts.nodename;

    if (unshare(CLONE_NEWUTS) < 0) {
      ProcError("unshare");
    }

    string new_name = string(child_ns_name_) + "-tmp";
    if (sethostname(new_name.c_str(), new_name.length()) < 0) {
      ProcError("sethostname");
    }
    
    // Retrieve and display hostname
    if (uname(&uts) < 0) {
      ProcError("uname");
    }
    LOG(INFO) << "PID " << my_pid_ << " parent post-unshare hostname() " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, new_name.c_str()), 0);

    return 0;
  }
  
  int WaitChildTerminate(void) {
    // Wait for child
    if (waitpid(child_pid_, NULL, 0) < 0) {
      ProcError("waitpid");
    }

    LOG(INFO) << "PID " << my_pid_ << " parent will now terminate";

    return 0;
  }

 private:
  const char *child_ns_name_;
  pid_t child_pid_;
  pid_t my_pid_;
  struct utsname uts;

  // Start function for cloned child
  static int TestUTSChildNS(void *arg)
  {
    struct utsname uts;
    pid_t  child_pid = getpid();
  
    // Retrieve and display hostname
    if (uname(&uts) < 0) {
      ProcError("uname");
    }
    LOG(INFO) << "PID " << child_pid << " child pre-sethostname(): " << uts.nodename;

    // Change hostname in UTS namespace of child
    const char *child_ns_name = const_cast<char *>(static_cast<char *>(arg));
    
    if (sethostname(child_ns_name, strlen(child_ns_name)) < 0) {
      ProcError("sethostname");
    }
    
    // Retrieve and display hostname
    if (uname(&uts) < 0) {
      ProcError("uname");
    }
    
    LOG(INFO) << "PID " << child_pid << " child post-sethostname(): " << uts.nodename;
    CHECK_EQ(strcmp(uts.nodename, child_ns_name), 0);
    
    // Keep the namespace open for a while, by sleeping.
    // This allows some experimentation-- 
    // for example, another process might join the namespace.
    sleep(2);

    // Terminates child
    LOG(INFO) << "PID " << child_pid << " child has terminated";
    
    return 0;           
  }

  static void ProcError(const char *str) {
    LOG(ERROR) << str << ": " << strerror(errno);
    throw SystemError(str);
  }
};

int
main(int argc, char *argv[])
{
  Init::InitEnv(&argc, &argv);

  UtsNSTester utns("Ory");
  utns.DumpUTSParentNS();
  utns.TestUTSSetNS();
  utns.TestUTSUnshareNS();
  utns.WaitChildTerminate();
  
  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

#endif // _GNU_SOURCE
