#include "CXXR/NumericVector.hpp"
#include <limits>
#include <cmath>
#include <R_ext/Arith.h>
#include <Defn.h>
#include <exception>

namespace CXXR {

//DEPENDS
template<>
double NumericVector<int,INTSXP>::Modulo::myfmod(double x1, double x2){
    double q = x1 / x2, tmp;

    if (x2 == 0.0) return std::numeric_limits<double>::quiet_NaN();
    tmp = x1 - floor(q) * x2;
    if(R_FINITE(q) && (fabs(q) > 1/R_AccuracyInfo.eps))
	warning(_("probable complete loss of accuracy in modulus"));
    q = floor(tmp/x2);
    return tmp - q * x2;
}

template<>
double NumericVector<double,REALSXP>::Power::myfmod(double x1, double x2){
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

template<>
double NumericVector<double, REALSXP>::Power::op(double l, double r)throw(RangeException){
	return R_pow(l, r);
}

template<>
int NumericVector<int, INTSXP>::Modulo::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value() || r==0){
		return NA_value();
	}
	return (l > 0 && r > 0) ? l % r : int(myfmod(double(l),double(r)));
}

} /* CXXR namespace */
