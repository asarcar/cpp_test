// Standard C++ Headers
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <string>       // stoi stoi stod

// Standard C Libraries
#include <cstddef>      // std::size_t;
#include <cassert>      // assert
// Google Libraries
#include <gflags/gflags.h>  // Parse command line args and flags
#include <glog/logging.h>   // Daemon Log function

// Memcached headers
#include <libmemcached/memcached.hh>

// Google Flag Declarations
DECLARE_string(mc_type);
DECLARE_string(mc_key);
DECLARE_string(mc_val);
DECLARE_int32(val_size);
DECLARE_int32(key_begin);
DECLARE_int32(key_range); 
DECLARE_int32(loop_count);
DECLARE_string(server_addr);
DECLARE_int32(server_port);

typedef long long int mc_llint_t;
typedef enum {MC_SET=0, MC_GET, MC_MGET, MC_LOOP_MGET, MC_MSET} eMcTestType;
static void memcache_process(void);
static void memcache_ops(Memcached &mc);
static eMcTestType get_mc_test_type(const std::string& mc_type); 
const mc_llint_t MC_MAX_VAL_DISP = 20; 
const int MC_MAX_KEY_SIZE=15;

int main(int argc, char *argv[])
{
  const std::string USAGE =
      std::string("Exercises memcached using various set/get options.\n") +
      std::string("Sample usage: ") + std::string(argv[0]) + 
      std::string(" [flags]");
  google::SetUsageMessage(USAGE);
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);
  // ::testing::InitGoogleTest(&argc, argv);
  // Initialize Google's flag library
  google::ParseCommandLineFlags(&argc, &argv, true);

  DLOG(INFO) << "Program args: " 
             << "mc_test " << FLAGS_mc_type
             << "; key " << FLAGS_mc_key 
             << "; val " << FLAGS_mc_val 
             << "; val_size " << FLAGS_val_size 
             << "; key_begin " << FLAGS_key_begin 
             << "; key_range " << FLAGS_key_range 
             << "; loop_count " << FLAGS_loop_count
             << "; server_addr " << FLAGS_server_addr 
             << "; server_port " << FLAGS_server_port 
             << std::endl;
  
  memcache_process();

  return 0;
}

static void memcache_process(void) {
  // Create a memcache object & add the server to the memcache object
  memcached_st memc = memcached_st();
  memcached_create(&memc);
  memcached_return rc = memcached_server_add(&memc, FLAGS_server_addr.c_str(), 
                                             FLAGS_server_port);
  DLOG_IF(INFO, rc == MEMCACHED_SUCCESS) 
      << "Added server/port (" <<FLAGS_server_addr 
      << "/" << FLAGS_server_port 
      << ") successfully" <<  std::endl;
  DLOG_IF(WARNING, rc != MEMCACHED_SUCCESS) 
      << "Couldn't add server/port: (" 
      << FLAGS_server_addr << "/" 
      << FLAGS_server_port << 
      ") error_code: " << rc << std::endl;

  Memcached mc(&memc);
  memcached_free(&memc);

  memcache_ops(mc); 

  return;
}

static void memcache_ops(Memcached &mc) {
  memcached_return rc;

  switch(get_mc_test_type(FLAGS_mc_type)) {

    case MC_SET: {
      rc = mc.set(FLAGS_mc_key.c_str(), 
                  FLAGS_mc_val.c_str(), 
                  FLAGS_mc_val.size());
      DLOG_IF(INFO, rc == MEMCACHED_SUCCESS) 
          << "Set Key/Val: (" << FLAGS_mc_key << "/" 
          << FLAGS_mc_val << ") successful" << std::endl;
      DLOG_IF(WARNING, rc != MEMCACHED_SUCCESS) 
          << "FAILED: set key: (" << FLAGS_mc_key 
          << "): errcode: " << rc << std::endl;
      break;
    }

    case MC_GET: {
      std::size_t val_len;
      const char *val = mc.get(FLAGS_mc_key.c_str(), &val_len);
      DLOG_IF(INFO, val != NULL) 
          << "Get Key/Val: (" << FLAGS_mc_key << "/" 
          << std::string(val) << ") successful" << std::endl;
      DLOG_IF(WARNING, val == NULL) 
          << "FAILED: get key: (" << FLAGS_mc_key << ")" << std::endl;
      break;
    }

    case MC_MGET: 
    case MC_LOOP_MGET: {
      // prepare parameters: keys/key_length, ... 
      // allocate keys, key_lengths, ...
      char **keys = new char*[FLAGS_key_range];
      std::size_t *key_lengths = new std::size_t[FLAGS_key_range];
      assert(keys);
      assert(key_lengths);
      for (int i=0; i < FLAGS_key_range; ++i) {
        keys[i] = new char[MC_MAX_KEY_SIZE+1];
        assert(keys[i]);
        sprintf(keys[i], "%d", i);
        key_lengths[i] = strlen(keys[i]);
      }

      // mget operation
      for (int lc=0; lc < FLAGS_loop_count; ++lc) {
        rc = mc.mget(keys, key_lengths, FLAGS_key_range);
        DLOG_IF(INFO, rc == MEMCACHED_SUCCESS) 
            << "[" << lc << "]: mget success: #keys= " 
            << FLAGS_key_range 
            << " key[0]/key_length[0]=(" << keys[0] << "/"
            << key_lengths[0] << ")" << std::endl;
        DLOG_IF(WARNING, rc != MEMCACHED_SUCCESS) 
            << "[" << lc << "]: mget FAILED: #keys= " << FLAGS_key_range 
            << " key[0]/key_length[0]=(" << keys[0] << "/"
            << key_lengths << ")" << std::endl;
        
        // fetch individual keys and log the value
        char ret_key[MEMCACHED_MAX_KEY];
        char *ret_val;
        std::size_t ret_key_length, ret_val_length;
        if (rc == MEMCACHED_SUCCESS) {
          int i = 0;
          while ((ret_val = mc.fetch(ret_key, &ret_key_length, 
                                     &ret_val_length)) != NULL) {
            ++i;
            DLOG(INFO) << "[" << lc 
                       << "]: Get [" << i << "] Key<len>/Val<len>: (" 
                       << ret_key << "<" << ret_key_length << ">/"
                       << ret_val << "<" << ret_val_length 
                       << ">) successful" << std::endl;
          }
        }
      }
      // free resources: memory
      for (int i=0; i < FLAGS_key_range; ++i) {
        delete[] keys[i];
      }
      delete[] key_lengths;
      delete[] keys;
      break;
    }

    case MC_MSET: {
      char *val = new char[FLAGS_val_size+1];
      assert(val != NULL);

      // prepare value: init val to a string of '1's
      for (int i = 0; i < FLAGS_val_size; ++i)
        val[i] = '1';
      val[FLAGS_val_size] = '\0'; // null terminate
      std::string val_disp = 
          (FLAGS_val_size < MC_MAX_VAL_DISP) ? val: 
          ("value_length=" + std::to_string(MC_MAX_VAL_DISP));

      std::string key;
      for (int i = FLAGS_key_begin; 
           i < (FLAGS_key_begin + FLAGS_key_range);
           ++i) {
        key = std::to_string(static_cast<mc_llint_t>(i));
        rc = mc.set(key.c_str(), val, FLAGS_val_size);
        DLOG_IF(INFO, rc == MEMCACHED_SUCCESS) 
            << "Set Key/Val: (" << key << "/" 
            << val_disp << ") successful" << std::endl;
        DLOG_IF(WARNING, rc != MEMCACHED_SUCCESS) 
            << "FAILED: set key: (" << key 
            << "): errcode: " << rc << std::endl;
      }
      
      delete[] val;

      break;
    }

    default: {
      break;
    }
  }

  return;
}

static eMcTestType get_mc_test_type(const std::string& mc_type) {
  if (mc_type == std::string("mset"))
    return MC_MSET;
  if (mc_type == std::string("mget"))
    return MC_MGET;
  if (mc_type == std::string("loop_mget"))
    return MC_LOOP_MGET;
  if (mc_type == std::string("set"))
    return MC_SET;
  return MC_GET;
}

const char defval[] = "keyvalue";
DEFINE_string(mc_type, "set", "set, get, mget, loop_mget, or mset");
DEFINE_string(mc_key, "keystring", "key to be used for get or set operation");
DEFINE_string(mc_val, defval, "value to be used for set operation");
DEFINE_int32(val_size, strlen(defval), "number of bytes for each value inserted in memcached"); 
DEFINE_int32(key_begin, 1, "first key value to use for mset or mget");
DEFINE_int32(key_range, 1000, "number of keys used for mset or mget");
DEFINE_int32(loop_count, 1, "number of times the mget entries are looked up in memcached"); 
DEFINE_string(server_addr, "192.168.0.2", "ip address of the memcached server");
DEFINE_int32(server_port, 11211, "port of the server used as memcached server");
