#if 0
g++ -o NumericVector -I../../src/include NumericVector.cpp
exit;
#endif

#include <iostream>
#include <cstdlib>
#include "CXXR/NumericVector.h"
using namespace CXXR;
int main(){
	NumericVector<int, INTSXP>* numericInt = new NumericVector<int, INTSXP>;
    return EXIT_SUCCESS;
}

