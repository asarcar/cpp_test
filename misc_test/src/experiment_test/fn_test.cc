#include <functional>  // std::function, less
#include <iomanip>     // std::left, right, setw, setfill...
#include <iostream>    // std::cout ...


struct TwoVal {
  int i;
  int j;
  TwoVal(int ix, int jx) : i{ix}, j{jx} {}
  TwoVal(const TwoVal& v) : i{v.i}, j{v.j} {}
};

namespace std {
template <>
struct greater<TwoVal> {
  bool operator()(const TwoVal& a, const TwoVal& b) const {
    return (a.i > b.i) || ((a.i == b.i) && (a.j > b.j));
  }
};
}

using namespace std;

template <typename T>
class Cmp {
 public:
  Cmp(const T& val) : base_{val} {}
  bool operator()(const T& val) const {
    return greater<T>{}(val, base_);
  }
 private:
  T base_;
};

int main(void) {
  Cmp<int>    fp{10};
  Cmp<TwoVal> fc{TwoVal{20,30}};

  function<bool(const int&)> fp2 = fp;
  function<bool(const TwoVal&)> fc2 = fc;

  string s1("fp{10}");
  cout << s1 << endl;
  cout << string(55, '-') << endl;
  cout << s1 << left << setw(40) << " < fp(11)  " << boolalpha << fp(11) << endl;
  cout << s1 << left << setw(40) << " < fp2(09) " << boolalpha << fp2(9) << endl;
  cout << s1 << left << setw(40) << " < fp2(10) " << boolalpha << fp2(10) << endl;
  cout << s1 << left << setw(40) << " < fp2(07) " << boolalpha << fp2(7) << endl;
  cout << string(55, '=') << endl;

  string s2("fc{TwoVal{20,30}}");
  cout << "s2" << endl;
  cout << string(55, '-') << endl;
  cout << s2 << left << setw(29) << " < fc(TwoVal{10,40}))  " 
       << boolalpha << fc({10,40}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{25,25})) " 
       << boolalpha << fc2({25,25}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{20,40})) " 
       << boolalpha << fc2({20,40}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{20,25})) " 
       << boolalpha << fc2({20,25}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{20,30})) " 
       << boolalpha << fc2({20,30}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{22, 28}) " 
       << boolalpha << fc2(TwoVal{22,28}) << endl;
  cout << string(55, '=') << endl;

  return 0;
}

