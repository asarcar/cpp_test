/* userns_child_exec.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Create a child process that executes a shell command in new
   namespace(s); allow UID and GID mappings to be specified when
   creating a user namespace.
*/
#ifdef _GNU_SOURCE

// C++ Standard Headers
#include <fstream>          // ifstream, ofstream
#include <iostream>        
#include <sstream>
// C Standard Headers
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <pwd.h>            // getpw_nam_r
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

class SystemError {
 public:
  SystemError(const char *str) : str_(str){}
 private:
  const char *str_;
};

class UidNSTester {
 public:

  // TEST: 
  // Assume 
  // a. Process Name space is spawned by UID_X and GID_X
  // b.  and argv[2] are valid user-ids passed to the program.
  //
  // 1. Fork C1 (PID1) in a new UID_NS_1. Set a mapping record 
  //    in the parent NS so that in uid in UID_NS_1 is uid1
  // 2. Fork C2 (PID2) in a new UID_NS_2. Set a mapping record 
  //    in the parent NS so that in uid in UID_NS_2 is uid2.
  // 3. Validate that when PID2 reads /proc/PID1/uid_map: it finds entry 
  //    UID_NS_2         UID_NS1         1 entry.
  // 4. Validate that when PID1 reads /proc/PID2/uid_map: it finds entry 
  //    UID_NS_1         UID_NS2         1 entry.
  int TestUidNS(const uid_t uid1, const uid_t uid2) {
    int pipe_child1[2];
    int pipe_child2[2];

    if (pipe(pipe_child1) < 0) {
      ProcError("pipe1");
    }
    if (pipe(pipe_child2) < 0) {
      ProcError("pipe2");
    }
    // 1. && 2.
    pid_t cpid1 = CloneUidNS(pipe_child1);
    UpdateUidMap(cpid1, uid1);
    pid_t cpid2 = CloneUidNS(pipe_child2);
    UpdateUidMap(cpid2, uid2);

    // Send cpid2, uid2 to C1 && cpid1, uid1 to C2
    SignalUpdateUidMapDone(pipe_child1, cpid2, uid2);
    SignalUpdateUidMapDone(pipe_child2, cpid1, uid1);

    ReapChild(cpid1);
    ReapChild(cpid2);
    
    return 0;
  }

  static int SignalUpdateUidMapDone(int *pipe_fd, pid_t pid, uid_t uid) {
    int pid_siz = sizeof(pid);
    int uid_siz = sizeof(uid);
    // We are write end: close the read end
    close(pipe_fd[READ]);

    if (write(pipe_fd[WRITE], &pid, pid_siz) < pid_siz) {
      ProcError("write-pid");
    }
    if (write(pipe_fd[WRITE], &uid, uid_siz) < uid_siz) {
      ProcError("write-uid");
    }

    // Now that we are done: close the write end
    close(pipe_fd[WRITE]);

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
  
  static int UpdateUidMap(pid_t cpid, uid_t uid) {
    ostringstream oss;
    oss << "/proc/" << cpid << "/uid_map";

    pid_t pid = getpid();
    uid_t uid_parent = getuid();

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

  // 1. Close WRITE end of PIPE as Child is READ end.
  // 2. Wait until you READ the (PID_SIB, UID_SIB) i.e. 
  //    PID and USER_ID of the sibling process.
  // 3. Read /proc/PID_SIB/uid_map record and validate that the record
  //    UID_SIB UID_MY 1
  static int UidNSFunc(void *arg) {
    int *pipe_fd = static_cast<int *>(arg);

    // We are read end. Close write end
    close(pipe_fd[WRITE]);

    pid_t pid_my = getpid();
    uid_t uid_my = getuid();
    LOG(INFO) << "PID " << pid_my << " UID " << uid_my 
              << " reading pid_sib & uid_sib from pipe";

    // Read the pid, uid coming from parent
    uid_t uid_sib;
    int   uid_sib_siz = sizeof(uid_sib);
    pid_t pid_sib;
    int   pid_sib_siz = sizeof(pid_sib);
    if (read(pipe_fd[READ], &pid_sib, pid_sib_siz) < pid_sib_siz) {
      ProcError("read-pid");
    }
    if (read(pipe_fd[READ], &uid_sib, uid_sib_siz) < uid_sib_siz) {
      ProcError("read-uid");
    }

    close(pipe_fd[READ]);

    LOG(INFO) << "PID " << pid_my << " read "
              << " UID_SIB " << uid_sib << " PID_SIB " << pid_sib;

    ostringstream oss;
    oss << "/proc/" << pid_sib << "/uid_map";

    LOG(INFO) << "PID " << " reading uid_map file " << oss.str();
    ifstream inp;
    string line;
    
    // Read file: the file should have only one line. line format
    // uid_sib uid_my 1
    inp.open(oss.str(), std::ifstream::in);
    CHECK(inp);
    CHECK(getline(inp, line));
    LOG(INFO) << "PID " << " read uid_map file " << oss.str() 
              << " line: " << line;

    istringstream ss(line);
    uid_t uida{0}, uidb{0};
    pid_t pid{0};
    ss >> uida >> uidb >> pid;
    LOG(INFO) << "PID " << " uid_map file " << oss.str() 
              << " parsed line: uida " << uida 
              << " uidb " << uidb << " pid_range " << pid;
    
    CHECK_EQ(uida, uid_sib);
    CHECK_EQ(uidb, uid_my);
    CHECK_EQ(pid, 1);

    // confirm previous was the only line
    CHECK(!getline(inp, line));

    inp.close();

    // When the process exits, the /proc/PID/uid_map file ceases
    // to exist. Sleep for 10ms before exiting: allows both children to read 
    // the uid_map file and validate the test before exiting.
    // A more clean way to do the same is C1 sends a message to C1 
    // and C2 sends a message to C1 once they are done. This way
    // both do not exit while running the tests and reading /proc/PID files
    usleep(10000);
    
    return 0;
  }

  // PID Clone NS and return PID of the child in the parent NS 
  static pid_t CloneUidNS(int *pipe_fd) {
    // Create a child that has its own PID namespace;
    // the child commences execution in childFunc()
    pid_t child_pid = clone(UidNSFunc, 
                            // Points to start of downwardly growing stack
                            new_stack(),
                            CLONE_NEWUSER | SIGCHLD, pipe_fd);
    if (child_pid < 0) {
      ProcError("clone");
    }

    LOG(INFO) << "PID: Parent " << getpid() << ": Clone Child " << child_pid 
              << ": GrandParent " << getppid();

    return child_pid;
  }
  
  static void ProcError(const char *str) {
    LOG(ERROR) << str << ": " << strerror(errno);
    throw SystemError(str);
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
};

// getpwnam has a bug in current version of linux in ubuntu/trusty.
// for now we are assuming that user guest exists in the system
//
// static bool ValidateUserName(const char *username) {
//   return (getpwnam(username) != nullptr);
// }
//
// string usage("This program validates user-id namespace. Sample usage:\n");
// usage += argv[0];
// usage += " valid-username1 valid-username2";
//   google::SetUsageMessage(usage);
// if (!((argc == 2) && ValidateUserName(argv[1]) && ValidateUserName(argv[2]))) {
//    cerr << usage << endl;
//    exit(EXIT_FAILURE);
// }

// Flag Declarations
DECLARE_bool(auto_test);

int
main(int argc, char *argv[])
{
  Init::InitEnv(&argc, &argv);

  UidNSTester uid_ns;
  // Currently: we are passing two existing uid of the system on this machine:
  // (root, guest). This hardcoding is abominal as one should just be allowed
  // to pass two usernames which the program should validate (using getpwnam)
  // and then run the test. Since getpwnam() has bug in ubuntu/trusty, we 
  // are taking this disgusting short cut.
  uid_ns.TestUidNS(0, 1001);
  
  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

#endif // _GNU_SOURCE
