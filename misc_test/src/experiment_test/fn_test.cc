#include <functional>  // std::function, less
#include <iomanip>     // std::left, right, setw, setfill...
#include <iostream>    // std::cout ...


struct TwoVal {
  int i;
  int j;
  TwoVal(int ix, int jx) : i{ix}, j{jx} {}
  TwoVal(const TwoVal& v) : i{v.i}, j{v.j} {}
};

const static TwoVal gkBase{20,30};

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

bool IsGreater(const TwoVal& val) {
  return greater<TwoVal>{}(val, gkBase);
}

int main(void) {
  Cmp<int>    fp{10};
  Cmp<TwoVal> fc{gkBase};

  /**
   * MORAL:
   * Function is a holder for any kind of function, 
   * function object or closure of an appropriate type. 
   * The "type" of function appropriate for us is something 
   * taking const int& or const TwoVal& and returning a bool:
   * e.g. std::function<bool(const TwoVal&)>
   * We can assign to it any function pointer or function object 
   * with a matching function signature:
   * Not only is predicate able to "hold" any function-like 
   * entity (of an appropriate function signature), but is also 
   * a value: it can be copied or assigned to. 
   * As is illustrated, it can be passed by value with no risk of 
   * slicing - we added extra field (base_) in this case   
   * because it guarantees to make a deep copy of the underlying 
   * concrete object. We use it just as any other function...
   */

  string s1("fp{10}");
  cout << s1 << endl;
  cout << string(55, '-') << endl;
  cout << s1 << left << setw(40) << " < fp(11)  " << boolalpha << fp(11) << endl;

  function<bool(const int&)> fp2 = fp;
  cout << "fp2: function<bool(const int&)> fp2 = fp" << endl;
  cout << s1 << left << setw(40) << " < fp2(09) " << boolalpha << fp2(9) << endl;
  cout << s1 << left << setw(40) << " < fp2(10) " << boolalpha << fp2(10) << endl;
  cout << s1 << left << setw(40) << " < fp2(07) " << boolalpha << fp2(7) << endl;
  cout << string(55, '=') << endl;

  string s2("fc{gkBase}");
  cout << "gkBase = TwoVal{20,30} = " << s2 << endl;
  cout << string(55, '-') << endl;
  cout << s2 << left << setw(29) << " < fc(TwoVal{10,40}))  " 
       << boolalpha << fc({10,40}) << endl;

  function<bool(const TwoVal&)> fc2 = fc;
  cout << "fc2: function<bool(const TwoVal&)> fc2 = fc" << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{25,25})) " 
       << boolalpha << fc2({25,25}) << endl;
  cout << s2 << left << setw(29) << " < fc2(TwoVal{20,40})) " 
       << boolalpha << fc2({20,40}) << endl;
  
  function<bool(const TwoVal&)> fc3 = &IsGreater;
  cout << "fc3: function<bool(const TwoVal&)> fc3 = &IsGreater" << endl;
  cout << "     bool IsGreater(const TwoVal& val) {" << endl
       << "       return greater<TwoVal>{}(val, gkBase);" << endl
       << "     }" << endl;
  cout << s2 << left << setw(29) << " < fc3(TwoVal{20,25})) " 
       << boolalpha << fc3({20,25}) << endl;
  
  function<bool(const TwoVal&)> fc4 = [](const TwoVal &val) {
    return greater<TwoVal>{}(val, gkBase);
  };
  cout << "fc4: function<bool(const TwoVal&)> fc3 = [](const TwoVal &val) {" << endl
       << "       return greater<TwoVal>{}(val, gkBase);" << endl
       << "     };" << endl;
  cout << s2 << left << setw(29) << " < fc4(TwoVal{20,30})) " 
       << boolalpha << fc4({20,30}) << endl;
  cout << s2 << left << setw(29) << " < fc4(TwoVal{22, 28}) " 
       << boolalpha << fc4(TwoVal{22,28}) << endl;
  cout << string(55, '=') << endl;

  return 0;
}

