#include "CXXR/NumericVector.hpp"
using namespace CXXR;

//NA_value specilisations.
//for integer, return minimum int.
template<>
int NumericVector<int,INTSXP>::NA_value(){
	return INT_MIN;
}
//Specilisation for double.
template<>
double NumericVector<double,REALSXP>::NA_value(){
	volatile ieee_double x;
	x.word[ieee_hw] = 0x7ff00000;
	x.word[ieee_lw] = 1954;
	return x.value;
}
//staticTypeName specilisations.
template<>
const char* NumericVector<int,INTSXP>::staticTypeName(){
	return "integer";
}
template<>
const char* NumericVector<double,REALSXP>::staticTypeName(){
	return "numeric";
}

//Subtract specilisations.
//int
template<>
int NumericVector<int,INTSXP>::Subtract::op(int l, int r){
	if(l==NumericVector<int,INTSXP>::NA_value() ||
			r==NumericVector<int,INTSXP>::NA_value()){
		return NumericVector<int,INTSXP>::NA_value();
	}
	int result = l-r;
	if(!GOODIDIFF(l,r,result)){
		result = NumericVector<int,INTSXP>::NA_value();
	}
	return result;
}
//double
template<>
double NumericVector<double,REALSXP>::Subtract::op(double l, double r){
	if(l==NumericVector<double,REALSXP>::NA_value() ||
			r==NumericVector<double,REALSXP>::NA_value()){
		return NumericVector<double,REALSXP>::NA_value();
	}
	double result = l-r;
	//TODO: range checks...?
	return result;
}

