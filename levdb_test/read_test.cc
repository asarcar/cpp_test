#include <iostream>
#include <cassert>

#include <leveldb/db.h>

int main() {
  leveldb::DB *p_db;
  leveldb::Options options;

  leveldb::Status status = leveldb::DB::Open(options, "./leveldb_dir", &p_db);
  assert(status.ok());

  leveldb::ReadOptions roptions;

  leveldb::Iterator *p_it = p_db->NewIterator(roptions);

  std::cout << "Reading Level DataBase" << std::endl;
  for(p_it->SeekToFirst(); p_it->Valid(); p_it->Next()) {
    leveldb::Slice s = p_it->key();
    leveldb::Slice t = p_it->value();

    std::cout << "<" << s.ToString() << ", " << t.ToString() << ">" << std::endl; 
  }
  std::cout << "Completed Reading Level DataBase" << std::endl;
  
  delete p_it;
  delete p_db;

  return 0;
}
