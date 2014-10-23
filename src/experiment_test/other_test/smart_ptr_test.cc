#include <iostream>  // cout ...
#include <memory>    // shared_ptr
#include <sstream>   // ostringstream
#include "utils/basic/basictypes.h"

using namespace std;

struct SmartPtrDeleter {
  void operator()(SmartPtrDeleter *p){delete p;}
};

struct IntDeleter {
  void operator()(int *p){delete p;}
};

struct IntDeleterPlus : public IntDeleter {
  size_t     dummy;
};

/**
 * Inheritance: vtable pointer takes 8 bytes
 * shared_ptr<>: takes 16 bytes as it contains two pointers:
 * 1) pointer to object 
 * 2) pointer to tracker object used for reference counting, 
 *    proper destruction etc. 
 *    The structure of the tracker object is of course an 
 *    implementation detail, but it is a few dozen bytes 
 *    (virtual table, pointer to the object, reference count, 
 *     weak count and some extra flags and whatnot).
 * If you use make_shared() as recommended, both the object 
 * itself and the information helding one will be created 
 * in one heap block and the pointers will be just a few bytes apart.
 */
class SmartPtrTest {
 public:
  using ShPtr = typename std::shared_ptr<SmartPtrTest>;
  using UnPtr = typename std::unique_ptr<SmartPtrTest>;
  SmartPtrTest(int a, int b, int c): 
    a_{a}, b_{b}, 
    p_{make_shared<int>(c)},
    p2_{unique_ptr<int>{new int{c}}} {}
  string toString(void) {
    ostringstream oss;
    oss << "a_ " << a_ << ": b_ " << b_ << ": p_ " << p_;
    if (p_ != nullptr) 
      oss << " shared_ptr_value " << *p_;
    oss << ": p2_ " << p2_.get();
    if (p2_ != nullptr) 
      oss << " unique_ptr_value " << *p2_;
    return oss.str();
  }
  virtual ~SmartPtrTest() {}

 private:
  int a_, b_;
  shared_ptr<int> p_; 
  unique_ptr<int> p2_;
};

SmartPtrTest::ShPtr fun(int i) {
  if (i == 0)
    return nullptr;

  return make_shared<SmartPtrTest>(i, i, i);
}

int main(void) {
  SmartPtrTest::ShPtr sp1 = fun(1);
  SmartPtrTest::ShPtr sp2 = fun(0);
  SmartPtrTest::ShPtr sp3 = SmartPtrTest::ShPtr{new SmartPtrTest(2,3,4)};
  SmartPtrTest::UnPtr up1 = SmartPtrTest::UnPtr{new SmartPtrTest(4,5,6)};
  
  size_t *p1 = reinterpret_cast<size_t *>(&sp1);
  size_t *p2 = reinterpret_cast<size_t *>(&sp2);
  size_t *p3 = reinterpret_cast<size_t *>(&sp3);
  size_t *p4 = reinterpret_cast<size_t *>(&up1);

  cout << "fun(3) [sp1: make_shared] " << sp1.get() << " " << sp1->toString() << endl;
  cout << "sp1 bytes: ";
  for (size_t i=0; i < sizeof(sp1)/sizeof(size_t); ++i)
    cout << hex << static_cast<int>(*(p1 + i)) << ":";
  cout << endl;

  cout << "fun(0) [sp2: make_shared]" << sp2.get() << endl;
  cout << "sp2 bytes: ";
  for (size_t i=0; i < sizeof(sp2)/sizeof(size_t); ++i)
    cout << hex << static_cast<int>(*(p2 + i)) << ":";
  cout << endl;
  
  cout << "sp3 [shared_ptr native]" << sp3.get() << " " << sp3->toString() << endl;
  cout << "sp3 bytes: ";
  for (size_t i=0; i < sizeof(sp3)/sizeof(size_t); ++i)
    cout << hex << static_cast<int>(*(p3 + i)) << ":";
  cout << endl;

  cout << "up1 [unique_ptr]" << up1.get() << " " << up1->toString() << endl;
  cout << "up1 bytes: ";
  for (size_t i=0; i < sizeof(up1)/sizeof(size_t); ++i)
    cout << hex << static_cast<int>(*(p4 + i)) << ":";
  cout << endl;

  IntDeleter d{};
  shared_ptr<int> shp{new int{100}, d};
  unique_ptr<int, IntDeleter> unp{new int {100}, d};

  cout << dec << "sizeof(SmartPtrTest) " << sizeof(SmartPtrTest) 
       << ": sizeof(SmartPtrTest::ShPtr) " << sizeof(SmartPtrTest::ShPtr) 
       << ": sizeof(SmartPtrTest::UnPtr) " << sizeof(SmartPtrTest::UnPtr) 
       << ": sizeof(shared_ptr<int>) " << sizeof(shared_ptr<int>) 
       << ": sizeof(unique_ptr<int>) " << sizeof(unique_ptr<int>) 
       << ": sizeof(shared_ptr<int>(int, IntDeleter)) " << sizeof(shp)
       << ": sizeof(unique_ptr<int, IntDeleter>) " << sizeof(unp)
       << ": sizeof(IntDeleter) " << sizeof(IntDeleter)
       << ": StructSizeOf(IntDeleter) " << (sizeof(IntDeleterPlus) - sizeof(size_t))
       << endl;
      // sizeof return 1 even for 0 sized structure: working around the problem
  return 0;
}
