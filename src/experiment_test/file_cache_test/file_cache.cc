// Copyright 2014 Elastasy Inc.
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

// Copyright 2010-2012 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: Arijit Sarcar

#include <iostream>
// #include <glog/logging.h>
// #include <stdlib.h>

#include "file_cache.h"

using namespace std;

//-----------------------------------------------------------------------------
// Constructor. 'max_cache_entries' is the maximum number of files that can
// be cached at any time. If a background flusher thread is implemented, then
// 'dirty_time_secs' is the duration after which an unpinned, dirty file will
// be cleaned (flushed to disk) by a background thread.
FileCache::FileCache(int max_cache_entries, int dirty_time_secs) {
    return;
}

    
// Destructor. Flushes all dirty buffers and stops the background thread (if
// implemented).
FileCache::~FileCache() {
    return;
}

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
void FileCache::PinFiles(const std::vector<std::string>& file_vec) {
    return;
}


// Unpin one or more files that were previously pinned. It is ok to unpin
// only a subset of the files that were previously pinned using PinFiles().
// It is undefined behavior to unpin a file that wasn't pinned.
void FileCache::UnpinFiles(const std::vector<std::string>& file_vec) {
    return;
}


// Provide read-only access to a pinned file's data in the cache.
//
// It is undefined behavior if the file is not pinned, or to access the
// buffer when the file isn't pinned.
const char *FileCache::FileData(const std::string& file_name) {
    return nullptr;
}


// Provide write access to a pinned file's data in the cache. This call marks
// the file's data as 'dirty'. The caller may update the contents of the file
// by writing to the memory pointed by the returned value.
//
// It is undefined behavior if the file is not pinned, or to access the
// buffer when the file is not pinned.
char *FileCache::MutableFileData(const std::string& file_name) {
    return nullptr;
}
// Mark a file for deletion from the local filesystem. It may or may not be
// pinned. If it is pinned, then the deletion is delayed until after the file
// is unpinned.
void FileCache::DeleteFile(const std::string& file_name) {
    return;
}
 
/*
 * Time Example:
time_t start, end;
time(&start);
time(&end);
std::cout << difftime(end, start) << " seconds" << std::endl;
*/
