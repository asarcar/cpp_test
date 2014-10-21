// Standard C++ Headers
#include <iostream>
#include <iomanip>     // std::left, std::setw ...
// Standard C Headers
// Google Headers
// Local Headers
#include "utils/basic/fassert.h"

using namespace asarcar;
using namespace std;

class Animal {
 public:
  virtual string eat(void) {return string("food");}
 private:
  int age = 0;
};

enum class eVegDiet {GRASS, LEAVES}; 
enum class eNonVegDiet {FISH, DEER};

class Herbivore: public Animal {
 public:
  string eat(void) override {return string("plant");}
  virtual string call(void) {return string("sweet");}
 private:
  eVegDiet diet = eVegDiet::GRASS;
};

class Carnivore: public Animal {
 public:
  string eat(void) override {return string("meat");}
  virtual string call(void) {return string("scary");}
 private:
  eNonVegDiet diet = eNonVegDiet::FISH;
};

class Cat: public Carnivore {
 public:
  string call(void) override {return string("meow");}
 private:
  bool dangerous = false;
};

class Tiger: public Carnivore {
 public:
  string call(void) override {return string("growl");}
 private:
  bool dangerous = true;
};

int main(void) {
  Animal     a;
  Herbivore  h;
  Carnivore  c;
  Cat        cat;
  Tiger      tiger;

  Animal     *ah_p     = new Herbivore;
  Animal     *ac_p     = new Carnivore;
  Animal     *acat_p   = new Cat;
  Animal     *atiger_p = new Tiger;

  Carnivore  *ccat_p   = new Cat;
  Carnivore  *ctiger_p = new Tiger;

  /**
   * Operations allowed:
   * Child = Parent
   * Parent = Child
   * Reference/Ptr to Child = Reference/Ptr to Parent
   * Reference/Ptr to Parent = Reference/Ptr to Child
   */
  a = h; // 'h' content is "sliced" to match to 'a'
  // no match for 'operator=' (operand types are 'Cat' and 'Carnivore')
  // cat = c;
  Animal *ah2_p = &h;
  FASSERT(ah2_p != nullptr);
  // invalid conversion from 'Animal*' to 'Herbivore*'
  // Herbivore *ha_p = &a; 

  /**
   * MORAL:
   * The new virtual methods added by children and grandchildren 
   * can be handled by extending the vtable format from one base class. 
   * Note: vtables used by multiple parents (inheritances) cannot be combined.
   */
  cout << "Sizes:" << endl;
  cout << string(16, '-') << endl;
  cout << left << setw(16) << "Animal "     << sizeof(a)     << endl;
  cout << left << setw(16) << "Herbivore "  << sizeof(h)     << endl;
  cout << left << setw(16) << "Carnivore "  << sizeof(c)     << endl;
  cout << left << setw(16) << "Cat "        << sizeof(cat)   << endl;
  cout << left << setw(16) << "Tiger "      << sizeof(tiger) << endl;
  cout << endl;

  cout << "Eat: " << endl;
  cout << string(16, '-') << endl;
  cout << left << setw(24) << "AnimalHerbivore " << ah_p->eat() << endl;
  cout << left << setw(24) << "AnimalCarnivore " << ac_p->eat() << endl;
  cout << left << setw(24) << "AnimalCat "       << acat_p->eat() << endl;
  cout << left << setw(24) << "AnimalTiger "     << atiger_p->eat() << endl;
  cout << endl;

  cout << "Call: " << endl;
  cout << string(16, '-') << endl;
  cout << left << setw(24) << "CarnivoreCat "   << ccat_p->call() << endl;
  cout << left << setw(24) << "CarnivoreTiger " << ctiger_p->call() << endl;
  cout << endl;

  return 0;
}
