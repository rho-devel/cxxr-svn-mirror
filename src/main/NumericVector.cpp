#include "CXXR/NumericVector.hpp"
#include <limits>
#include <cmath>
#include <R_ext/Arith.h>
#include <Defn.h>
namespace CXXR {
//NA_value specilisations.
//for integer, return minimum int.
template<>
int NumericVector<int,INTSXP>::NA_value(){
	return std::numeric_limits<int>::min();
}
//Specilisation for double. Note: this creates and returns NaN.
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
	if(result == NumericVector<int,INTSXP>::NA_value() || !goodIDiff(l,r,result)){
		RangeException rangeException;
		throw rangeException;
		//result = NumericVector<int,INTSXP>::NA_value();
	}
	return result;
}
//double
template<>
double NumericVector<double,REALSXP>::Subtract::op(double l, double r)throw(RangeException){
	return l-r;
}
//complex
template<>
Rcomplex NumericVector<Rcomplex,CPLXSXP>::Subtract::op(Rcomplex l,Rcomplex r)throw(RangeException){
	l.r -= (r.r);
	l.i -= (r.i);
	return l;
}
//Add specilisation
//int
template<>
int NumericVector<int, INTSXP>::Add::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value()){
		return NA_value();
	}
	int result=l+r;
	if(result == NumericVector<int, INTSXP>::NA_value() || !goodISum(l,r,result)){
		RangeException rangeException;
		throw rangeException;
	}
	return result;
}
//double
template<>
double NumericVector<double,REALSXP>::Add::op(double l, double r)throw(RangeException){
	return l+r;
}
//complex
template<>
Rcomplex NumericVector<Rcomplex,CPLXSXP>::Add::op(Rcomplex l, Rcomplex r)throw(RangeException){
	l.r += r.r;
	l.i += r.i;
	return l;
}
//Divide specilisations
//there is no int version.
//real
template<>
double NumericVector<double, REALSXP>::Divide::op(double l, double r)throw(RangeException){
	return l/r;
}
//complex
// TODO - see complex.c

//multiply
//int
template<>
int NumericVector<int, INTSXP>::Multiply::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value()){
		return NA_value();
	}
	int result=l*r;
	if(result == NumericVector<int, INTSXP>::NA_value() || !goodIProd(l,r,result)){
		RangeException rangeException;
		throw rangeException;
	}
	return result;
}
//double
template<>
double NumericVector<double, REALSXP>::Multiply::op(double l, double r)throw(RangeException){
	return l*r;
}
//complex
//TODO

//Power
//-dependencies
template<>
double NumericVector<double,REALSXP>::myfmod(double x1, double x2){
    double q = x1 / x2, tmp;

    if (x2 == 0.0) return std::numeric_limits<double>::quiet_NaN();
    tmp = x1 - floor(q) * x2;
    if(R_FINITE(q) && (fabs(q) > 1/R_AccuracyInfo.eps))
	warning(_("probable complete loss of accuracy in modulus"));
    q = floor(tmp/x2);
    return tmp - q * x2;
}

template<>
double NumericVector<double ,REALSXP>::Power::R_pow(double x, double y){
	if(x == 1. || y == 0.)
		return(1.);
	if(x == 0.) {
		if(y > 0.) return(0.);
		else if(y < 0) return(std::numeric_limits<double>::infinity());
		else return(y); /* NA or NaN, we assert */
	}
	if (std::isfinite(x) && std::isfinite(y))
/* work around a bug in May 2007 snapshots of gcc pre-4.3.0, also
   present in the release version.  If compiled with, say, -g -O3
   on x86_64 Linux this compiles to a call to sqrtsd and gives
   100^0.5 as 3.162278.  -g is needed, as well as -O2 or higher.
   example(pbirthday) will fail.
 */
#if __GNUC__ == 4 && __GNUC_MINOR__ >= 3
	return (y == 2.0) ? x*x : pow(x, y);
#else
	return (y == 2.0) ? x*x : ((y == 0.5) ? sqrt(x) : pow(x, y));
#endif
	if (ISNAN(x) || ISNAN(y))
		return(x + y);
	if(!R_FINITE(x)) {
		if(x > 0)		/* Inf ^ y */
			return (y < 0.)? 0. : R_PosInf;
		else {			/* (-Inf) ^ y */
			if(R_FINITE(y) && y == floor(y)) /* (-Inf) ^ n */
				return (y < 0.) ? 0. : (myfmod(y, 2.) ? x  : -x);
		}
	}
	if(!R_FINITE(y)) {
		if(x >= 0) {
			if(y > 0)		/* y == +Inf */
				return (x >= 1) ? R_PosInf : 0.;
			else		/* y == -Inf */
				return (x < 1) ? R_PosInf : 0.;
		}
	}
	return(std::numeric_limits<double>::quiet_NaN());		/* all other cases: (-Inf)^{+-Inf,non-int}; (neg)^{+-Inf} */
}

} /* CXXR namespace */
