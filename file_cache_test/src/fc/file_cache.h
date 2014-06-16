// The problem is to implement a file cache in C++ that derives the interface
// given below in class FileCache. The typical usage is for a client to call
// 'PinFiles()' to pin a bunch of files in the cache and then either read or
// write to their in-memory contents in the cache. Writing to a cache entry
// makes that entry 'dirty'. Before a dirty entry can be evicted from the
// cache, it must be unpinned and has to be cleaned by writing the
// corresponding data to disk.
//
// All files are assumed to have size 10KB. If a file doesn't exist to begin
// with, it should be created and filled with zeros - the size should be 10KB.
//
// FileCache should be a thread-safe object that can be simultaneously
// accessed by multiple threads.

#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_

#include <string>
#include <vector>

class FileCache {
 public:
  // Constructor. 'max_cache_entries' is the maximum number of files that can
  // be cached at any time. If a background flusher thread is implemented, then
  // 'dirty_time_secs' is the duration after which an unpinned, dirty file will
  // be cleaned (flushed to disk) by a background thread.
  FileCache(int max_cache_entries, int dirty_time_secs);

  // Destructor. Flushes all dirty buffers and stops the background thread (if
  // implemented).
  ~FileCache();

  // Pins the given files in vector 'file_vec' in the cache. If any of these
  // files are not already cached, they are first read from the local
  // filesystem. If the cache is full, then some existing cache entries may be
  // evicted. If no entries can be evicted (e.g., if they are all pinned, or
  // dirty), then this method will block until a suitable number of cache
  // entries becomes available. It is fine if more than one thread pins the
  // same file, however the file should not become unpinned until both pins
  // have been removed from the file.
  //
  // Correct usage of PinFiles() is assumed to be the caller's responsibility,
  // and therefore deadlock avoidance doesn't need to be handled. The caller is
  // assumed to pin all the files it wants using one PinFiles() call and not
  // call multiple PinFiles() calls without unpinning those files in the
  // interim.
  void PinFiles(const std::vector<std::string>& file_vec);

  // Unpin one or more files that were previously pinned. It is ok to unpin
  // only a subset of the files that were previously pinned using PinFiles().
  // It is undefined behavior to unpin a file that wasn't pinned.
  void UnpinFiles(const std::vector<std::string>& file_vec);

  // Provide read-only access to a pinned file's data in the cache.
  //
  // It is undefined behavior if the file is not pinned, or to access the
  // buffer when the file isn't pinned.
  const char *FileData(const std::string& file_name);

  // Provide write access to a pinned file's data in the cache. This call marks
  // the file's data as 'dirty'. The caller may update the contents of the file
  // by writing to the memory pointed by the returned value.
  //
  // It is undefined behavior if the file is not pinned, or to access the
  // buffer when the file is not pinned.
  char *MutableFileData(const std::string& file_name);

  // Mark a file for deletion from the local filesystem. It may or may not be
  // pinned. If it is pinned, then the deletion is delayed until after the file
  // is unpinned.
  void DeleteFile(const std::string& file_name);

 protected:

 private:
  // Prevent unintended bad usage: copy/move ctor or copy/move assignable 
  FileCache(const FileCache &);
  FileCache(FileCache &&);
  void operator=(const FileCache &);
  void operator=(FileCache &&);


};

#endif // _FILE_CACHE_H_
