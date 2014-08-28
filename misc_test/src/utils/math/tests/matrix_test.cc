// Copyright 2014 asarcar Inc.
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
//
// Author: Arijit Sarcar <sarcar_a@yahoo.com>

// Standard C++ Headers
#include <iostream>
// Standard C Headers
#include <cmath>           // abs(double)
// Google Headers
#include <glog/logging.h>   
// Local Headers
#include "utils/basic/basictypes.h"
#include "utils/basic/fassert.h"
#include "utils/basic/init.h"
#include "utils/math/matrix.h"

using namespace asarcar;
using namespace asarcar::utils;
using namespace asarcar::utils::math;
using namespace std;

// Flag Declarations
DECLARE_bool(auto_test);

#if 0
using Mat2d = Matrix<double,2>;
using Vec   = Matrix<double,1>;

class EquationSolveTester {
 public:
  constexpr static double min_val = 0.01; 
  explicit EquationSolveTester(Mat2d& A, Vec& b, Vec& res) : 
      A_{A}, b_{b}, res_{res} {}
  EquationSolveTester(void) = delete;
  
  void ValidateClassicalGausianElimination() {
    ClassicalElimination(A_,b_);
    Vec result = BackSubstitution(A_, b_);
    // Match result and res_
  }

 private:
  Mat2d A_;
  Vec   b_;
  Vec   res_;

  void ClassicalElimination(Mat2d& A, Vec& b) {
    const size_t n = A.dim1();
    // traverse from 1st col to penultimate: fill 0s into all elements under diagonal
    for (int j=0; j < n-1; ++j) {
      const double pivot = A(j,j);
      FASSERT(abs(pivot) >= min_val);
      // fill zeros into each element under the diagonal ith row
      for (int i=j+1; i<n; ++i) {
        const double mult = A(i,j)/pivot;
        A[i](Slice(j)) = ScaleAndAdd(A[j](Slice(j)), -mult, A[i](Slice(j)));
        b(i) -= mult*b(j); // reflect change to b
      }
    }
  }
  
  Vec BackSubstitution(const Mat2d& A, const Vec& b) {
    const size_t n = A.dim1();
    Vec x(n);
    
    for (i = n-1; i>=0; --i) {
      double s = b(i) - dot_product(A[i](Slice(i+1)), x(Slice(i+1)));
      double m = A(i,i);
      FASSERT(m >= min_val);
      x(i) = s/m;
    }
    return x;
  }
};

#endif

int main(int argc, char **argv) {
  Init::InitEnv(&argc, &argv);

  // Slice Tests
  MatrixSlice<2> ms{0, {4, 5}, {5, 1}};
  LOG(INFO) << "ms{0,{4,5},{5,1}}: " << ms;
  CHECK_EQ(ms(0,0),0) << "ms(0,0) = " << ms(0,0) << " != 0";
  CHECK_EQ(ms(3,2),17) << "ms(3,2) = " << ms(3,2) << " != 17";

  MatrixSlice<2> ms2{3, 4};
  LOG(INFO) << "ms2{3,4}: " << ms2;
  CHECK_EQ(ms2(2,3),11) << "ms2(2,3) = " << ms2(2,3) << " != 11";

  // Matrix Initialization Tests
  Matrix<int, 2> m(4, 5);
  LOG(INFO) << "m(4,5): " << m;
  CHECK_EQ(m.size(), 20) << "m.size() = " << m.size() << " != 20";
  CHECK_EQ(m.extent(0), 4) << "m.extent(0) = " << m.extent(0) << " != 4";
  CHECK_EQ(m.stride(0), 5) << "m.stride(0) = " << m.stride(0) << " != 5";
  CHECK_EQ(m.extent(1), 5) << "m.extent(1) = " << m.extent(1) << " != 5";
  CHECK_EQ(m.stride(1), 1) << "m.stride(1) = " << m.stride(1) << " != 1";

  Matrix<int, 2> m2 {{1,2,3,4},{4,5,6,7},{1,2,3,4},{4,5,6,7}};
  LOG(INFO) << "m2 {{1,2,3,4},{4,5,6,7},{1,2,3,4},{4,5,6,7}} " << m2;
  CHECK_EQ(m2.size(), 16) << "m2.size = " << m2.size() << " != 16";
  CHECK_EQ(m2.extent(0), 4) << "m2.extent(0) = " << m2.extent(0) << " != 4";
  CHECK_EQ(m2.stride(0), 4) << "m2.stride(0) = " << m2.stride(0) << " != 4";
  CHECK_EQ(m2.extent(1), 4) << "m2.extent(1) = " << m2.extent(1) << " != 4";
  CHECK_EQ(m2.stride(1), 1) << "m2.stride(1) = " << m2.stride(1) << " != 1";

  Matrix<int, 3> m3 {
    // dim 0: 0
    {
      {1,2,3,4}, // dim 1: 0; dim 2: 0,1,2,3
      {2,3,4,5}  // dim 1: 1; dim 2: 0,1,2,3
    },
    // dim 0: 1
    {
      {4,5,6,7}, // dim 1: 0; dim 2: 0,1,2,3
      {5,6,7,8}  // dim 1: 1; dim 2: 0,1,2,3
    }
  };
  LOG(INFO) << "m3 {{{1,2,3,4},{2,3,4,5}},{{4,5,6,7},{5,6,7,8}}} " << m3;
  CHECK_EQ(m3.size(), 16) << "m3.size = " << m3.size() << " != 16";
  CHECK_EQ(m3.extent(0), 2) << "m3.extent(0) = " << m3.extent(0) << " != 2";
  CHECK_EQ(m3.stride(0), 8) << "m3.stride(0) = " << m3.stride(0) << " != 8";
  CHECK_EQ(m3.extent(1), 2) << "m3.extent(1) = " << m3.extent(1) << " != 2";
  CHECK_EQ(m3.stride(1), 4) << "m3.stride(1) = " << m3.stride(1) << " != 4";
  CHECK_EQ(m3.extent(2), 4) << "m3.extent(2) = " << m3.extent(2) << " != 2";
  CHECK_EQ(m3.stride(2), 1) << "m3.stride(2) = " << m3.stride(2) << " != 1";
  
  // MatrixType Tests
  LOG(INFO) << "IsMatrixType<Matrix<int, 3>>() " << boolalpha 
            << IsMatrixType<Matrix<int,3>>();
  CHECK( (IsMatrixType< Matrix<int,3> >()) ) 
      << "IsMatrixType<Matrix<int, 3>>() " << boolalpha
      << (IsMatrixType<Matrix<int,3>>()) << " != true";

  // MatrixRef Tests
  // MatrixSlice<2> ms{0, {4, 5}, {5, 1}};
  vector<int> elems(20);
  MatrixRef<int, 2> mr{ms, elems};
  LOG(INFO) << "mr{ms, m.elems_} " << mr;

  Matrix<int, 2> m4(mr);
  LOG(INFO) << "m4{mr} " << m4;
  CHECK_EQ(m4.extent(0), 4) << "m4.extent(0) = " << m4.extent(0) << " != 4";
  CHECK_EQ(m4.stride(0), 5) << "m4.stride(0) = " << m4.stride(0) << " != 5";
  CHECK_EQ(m4.extent(1), 5) << "m4.extent(1) = " << m4.extent(1) << " != 5";
  CHECK_EQ(m4.stride(1), 1) << "m4.stride(1) = " << m4.stride(1) << " != 1";

  // MatrixSlice<2> ms2{3, 4};
  MatrixRef<int, 2> mr2{ms2, elems};
  LOG(INFO) << "mr2{ms2, m.elems_} " << mr2;
  m2 = mr2;
  LOG(INFO) << "m2 = MatrixRef{ms2, m.elems_} " << m2;
  CHECK_EQ(m2.extent(0), 3) << "m2.extent(0) = " << m2.extent(0) << " != 3";
  CHECK_EQ(m2.stride(0), 4) << "m2.stride(0) = " << m2.stride(0) << " != 4";
  CHECK_EQ(m2.extent(1), 4) << "m2.extent(1) = " << m2.extent(1) << " != 4";
  CHECK_EQ(m2.stride(1), 1) << "m2.stride(1) = " << m2.stride(1) << " != 1";

  // Matrix Indexing
  LOG(INFO) << "m3(1,0,2) = " << m3(1,0,2);
  CHECK_EQ(m3(1,0,2), 6) << "m3(1,0,2) = " << m3(1,0,2) << " != 6";
  LOG(INFO) << "m3(Slice(1),0) = " << m3(Slice(1),1);

  return 0;
}

DEFINE_bool(auto_test, false, 
            "test run programmatically (when true) or manually (when false)");

