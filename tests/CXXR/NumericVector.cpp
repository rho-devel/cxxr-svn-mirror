#include <iostream>
#include <cstdlib>
#include "CXXR/NumericVector.hpp"
#include "CXXR/IntVector.h" 

#include "Defn.h"

using namespace CXXR;
using namespace std;

//subtract operation copy/paste from arithmatic.cpp (with some minor modification)
int *int_subtract_arith_cpp(int, int);
//compares the two differing subtract implementations.
bool int_compare_vectors(IntVector*, IntVector*);

int main(){
	IntVector* iv1 = new IntVector(20,0);
	IntVector* iv2 = new IntVector(20,0);
	(*iv1)[0] = 5;
	(*iv2)[0] = INT_MAX;
	(*iv1)[1] = 1;
	(*iv2)[1] = INT_MAX;
	(*iv1)[2] = 0;
	(*iv2)[2] = INT_MAX;
	(*iv1)[3] = INT_MIN;
	(*iv2)[3] = 1;
	(*iv1)[4] = INT_MAX;
	(*iv2)[4] = INT_MAX;
	(*iv1)[5] = INT_MAX;
	(*iv2)[5] = INT_MAX;
	int_compare_vectors(iv1,iv2);
	NumericVector<double, REALSXP>* numericReal = new NumericVector<double, REALSXP>(90);
	return EXIT_SUCCESS;
}
bool int_compare_vectors(IntVector* v1, IntVector* v2){
	for(int i=v1->size();i<v1->size();i++){
		int *result = new int;
		try{
			(*result)=NumericVector<int,INTSXP>::Subtract::op((*v1)[i],(*v2)[i]);
		}catch(...){//todo: specify exception.
			delete result;
			result = NULL;
		}
		int *result2=int_subtract_arith_cpp((*v1)[i],(*v2)[i]);
		cout << result << result2 << (result==result2) << endl;
	}
	return true;
}

# define GOODISUM(x, y, z) (double(x) + double(y) == (z))
# define GOODIDIFF(x, y, z) (double(x) - double(y) == (z))
/**
 * performs x1-x2, using arithmetic.cpp code extract.
 * @return pointer to integer, or NULL if naflag was made true.
 */
int *int_subtract_arith_cpp(int x1, int x2){
	int *ans = new int;
	bool naflag = FALSE;
	//x1 = INTEGER(s1)[i1];
	//x2 = INTEGER(s2)[i2];
	//note: was NA_INTEGER, changed to INT_MIN.
	if (x1 == INT_MIN || x2 == INT_MIN){
		//INTEGER(ans)[i]= NA_INTEGER;
		*ans = INT_MIN;
	}else {
		int val = x1 - x2;
		if (val != INT_MIN && GOODIDIFF(x1, x2, val))
			*ans = val;
		else {
			*ans = INT_MIN;
			naflag = TRUE;
		}
	}
	if (naflag){
		delete ans;
		return NULL;
		//warningcall(lcall, INTEGER_OVERFLOW_WARNING);
	}
	return ans;
}
