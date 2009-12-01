#include "CXXR/NumericVector.hpp"
#include <limits>
namespace CXXR {
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

//Subtract specilisations.
//int
template<>
int NumericVector<int,INTSXP>::Subtract::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value()){
		return NA_value();
	}
	int result = l-r;
	//if the result has become the internal value used for NA, or a range error has occured, throw the exception.
	if(result == NumericVector<int,INTSXP>::NA_value() || !GOODIDIFF(l,r,result)){
		RangeException rangeException;
		throw rangeException;
		//result = NumericVector<int,INTSXP>::NA_value();
	}
	return result;
}
//double
template<>
double NumericVector<double,REALSXP>::Subtract::op(double l, double r)throw(RangeException){
	if(l==NA_value() ||	r==NA_value()){
		return NA_value();
	}
	return l-r;
}
//complex
template<>
Rcomplex NumericVector<Rcomplex,CPLXSXP>::Subtract::op(Rcomplex l,Rcomplex r)throw(RangeException){
	l.r -= (r.r);
	l.i -= (r.i);
	return l;
}


} /* CXXR namespace */
