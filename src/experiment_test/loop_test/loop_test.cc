//! @file loop_test.cc
//! @author Arijit Ory Sarcar
//! @brief Use Array & Map Class

// Standard C++ Headers
#include <array>      // std::array
#include <map>        // std::map
#include <iostream>
// Standard C Headers
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"

#define NUM_ARRAY_ELEMENTS 3 //!< Array Size
typedef std::array<int, NUM_ARRAY_ELEMENTS> arrN; //!< array typedef

//! Main Function
//! @return int
int main() {
  arrN arr = {{1, 2, 3}};   
  arrN arr_sq = {{1, 4, 9}}; 
  std::map<std::string, arrN> map;

  map["base"] = arr;
  map["square"] = arr_sq;

  for (const auto &kvp : map) {
    std::cout << kvp.first << std::endl;

    for (const auto &v: kvp.second) {
      std::cout << v << std::endl;
    }
  }

  return 0;
}
