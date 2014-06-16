#include <iostream>
#include <cassert>

#include <leveldb/db.h>

int main() {
  leveldb::DB *p_db;
  leveldb::Options options;

  options.create_if_missing = true;

  leveldb::Status status = leveldb::DB::Open(options, "./leveldb_dir", &p_db);
  assert(status.ok());
  
  leveldb::WriteOptions woptions;

  std::cout << "Writing Level DataBase" << std::endl;

  leveldb::Slice s = "Hello";
  leveldb::Slice t = "World";
  p_db->Put(woptions, s, t);

  s = "Raja";
  t = "Maharaja";
  p_db->Put(woptions, s, t);

  std::cout << "Completed Writing Level DataBase" << std::endl;

  delete p_db;
  
  return 0;
}
