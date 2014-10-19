#include <iostream>  // cout ...
#include <memory>    // shared_ptr
#include <sstream>   // ostringstream

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
class ShpTest {
 public:
  using Ptr = typename std::shared_ptr<ShpTest>;
  ShpTest(int a, int b, int c): a_{a}, b_{b}, p_{std::make_shared<int>(c)} {}
  std::string toString(void) {
    std::ostringstream oss;
    oss << "a_ " << a_ << ": b_ " << b_ << ": p_ " << p_;
    if (p_ != nullptr) 
      oss << " value " << *p_;
    return oss.str();
  }
  virtual ~ShpTest() {}

 private:
  int a_, b_;
  std::shared_ptr<int> p_; 
};

ShpTest::Ptr fun(int i) {
  if (i == 0)
    return nullptr;

  return std::make_shared<ShpTest>(i, i, i);
}

int main(void) {
  std::cout << "fun(3) " << fun(3) << " " << fun(3)->toString() << std::endl;
  std::cout << "fun(0) " << fun(0) << std::endl;
  std::cout << "sizeof(ShpTest) " << sizeof(ShpTest) 
            <<  ": sizeof(std::shared_ptr<int>) " 
            << sizeof(std::shared_ptr<int>) << std::endl;
  return 0;
}
