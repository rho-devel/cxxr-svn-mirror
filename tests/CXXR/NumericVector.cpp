#include <iostream>
#include <cstdlib>
#include "CXXR/NumericVector.h"
using namespace CXXR;
int main(){
	NumericVector<int, INTSXP>* numericInt = new NumericVector<int, INTSXP>((size_t)1);
	return EXIT_SUCCESS;
}

