#include "CXXR/NumericVector.hpp"
#include <limits>
namespace CXXR {
//NA_value specilisations.
//for integer, return minimum int.
template<>
int NumericVector<int,INTSXP>::NA_value()throw(unexpectedContextException){
	return INT_MIN;
}
//Specilisation for double.
template<>
double NumericVector<double,REALSXP>::NA_value()throw(unexpectedContextException){
	volatile ieee_double x;
	x.word[ieee_hw] = 0x7ff00000;
	x.word[ieee_lw] = 1954;
	return x.value;
}

//Subtract specilisations.
//int
template<>
int NumericVector<int,INTSXP>::Subtract::op(int l, int r)throw(unexpectedContextException,rangeException){
	if(l==NumericVector<int,INTSXP>::NA_value() ||
			r==NumericVector<int,INTSXP>::NA_value()){
		return NumericVector<int,INTSXP>::NA_value();
	}
	int result = l-r;
	//if the result has become the internal value used for NA, or a range error has occured, throw the exception.
	if(result == NumericVector<int,INTSXP>::NA_value() || !GOODIDIFF(l,r,result)){
		throw new rangeException;
		//result = NumericVector<int,INTSXP>::NA_value();
	}
	return result;
}
//double
template<>
double NumericVector<double,REALSXP>::Subtract::op(double l, double r)throw(unexpectedContextException,rangeException){
	if(l==NumericVector<double,REALSXP>::NA_value() ||
			r==NumericVector<double,REALSXP>::NA_value()){
		return NumericVector<double,REALSXP>::NA_value();
	}
	return l-r;
}
//complex
template<>
Rcomplex NumericVector<Rcomplex,CPLXSXP>::Subtract::op(Rcomplex l,Rcomplex r)throw(unexpectedContextException,rangeException){
	Rcomplex result;
	result.r = (l.r - r.r);
	result.i = (l.i - l.r);
	return result;
}

} /* CXXR namespace */
