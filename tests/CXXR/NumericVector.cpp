#include <iostream>
#include <cstdlib>
#include "CXXR/NumericVector.hpp"
using namespace CXXR;
int main(){
	NumericVector<int, INTSXP>* numericInt = new NumericVector<int, INTSXP>(1);
	NumericVector<double, REALSXP>* numericReal = new NumericVector<double, REALSXP>(90);
	return EXIT_SUCCESS;
}

