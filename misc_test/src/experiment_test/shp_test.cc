#include <iostream>  // cout ...
#include <memory>    // shared_ptr
#include <sstream>   // ostringstream

class ShpTest {
 public:
  using Ptr = typename std::shared_ptr<ShpTest>;
  ShpTest(int a, int b, int c): a_{a}, b_{b}, c_{c} {}
  std::string toString(void) {
    std::ostringstream oss;
    oss << "a_ " << a_ << ": b_ " << b_ << ": c_ " << c_;
    return oss.str();
  }
  const static Ptr NullPtr;

 private:
  int a_, b_, c_;
};

const ShpTest::Ptr ShpTest::NullPtr;

ShpTest::Ptr fun(int i) {
  if (i == 0) {
    return ShpTest::NullPtr;
  }

  return std::make_shared<ShpTest>(i, i, i);
}

int main(void) {
  std::cout << "fun(3) " << fun(3) << " " << fun(3)->toString() << std::endl;
  std::cout << "fun(0) " << fun(0) << std::endl;
  return 0;
}
