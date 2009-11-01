#include "CXXR/NumericVector.h"
#include "Defn.h"

namespace CXXR{
	//constructors/deconstructor
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::NumericVector(size_t sz):
			DumbVector<T,ST>(sz){
	}
	template <typename T,SEXPTYPE ST>
	NumericVector<T,ST>::NumericVector(size_t sz, const T& initializer):
			DumbVector<T,ST>(sz,initializer){
	}
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::~NumericVector(){
	}
	//Generic case implementations.
	//currently don't do much. Maybe throw exceptions?
	template <typename T, SEXPTYPE ST>
	T NumericVector<T,ST>::NA_value(){
		return NULL;
	}
	template <typename T, SEXPTYPE ST>
	const char* NumericVector<T,ST>::staticTypeName(){
		return "SHOULD NOT HAPPEN.";
	}
	//Subtract sub-class.
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::Subtract::Subtract(){
	}
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::Subtract::~Subtract(){
	}
	template <typename T, SEXPTYPE ST>
	T NumericVector<T,ST>::Subtract::op(T l, T r){
		return NULL;
	}
	
	//Specilisation for int.
	template<>
	int NumericVector<int,INTSXP>::NA_value(){
		return INT_MIN;
	}
	
	template<>
	const char* NumericVector<int,INTSXP>::staticTypeName(){
		return "integer";
	}

	//Specilisation for double.
	template<>
	double NumericVector<double,REALSXP>::NA_value(){
		volatile ieee_double x;
		x.word[ieee_hw] = 0x7ff00000;
		x.word[ieee_lw] = 1954;
		return x.value;
	}
	/*template<>
	const char* NumericVector<double,REALSXP>::staticTypeName(){
		return "TODO";
	}*/

	//Subtract specilisation for int.
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

	// Naive subtract specilisation for double.
	template<>
	double NumericVector<double,REALSXP>::Subtract::op(double l, double r){
		if(l==NumericVector<double,REALSXP>::NA_value() ||
				r==NumericVector<double,REALSXP>::NA_value()){
			return NumericVector<double,REALSXP>::NA_value();
		}
		double result = l-r;
		//TODO: range checks?
		return result;
	}

	//taken from arithmetic.cpp, used for overflow and underflow checks.
	//however - is this style really apropriate here?
	#define USES_TWOS_COMPLEMENT 1
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::GOODIPROD(int x, int y, int z){
		return double(x) * double(y) == z;
	}
	#if USES_TWOS_COMPLEMENT
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::GOODISUM(int x, int y, int z){
		return ((x > 0) ? (y < z) : ! (y < z));
	}
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::OPPOSITE_SIGNS(int x, int y){
		return (x < 0)^(y < 0);
	}
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::GOODIDIFF(int x, int y, int z){
		return !(OPPOSITE_SIGNS(x, y) && OPPOSITE_SIGNS(x, z));
	}
	#else
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::GOODISUM(x, y, z){
		return(double(x) + double(y) == (z));
	}
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::GOODIDIFF(x, y, z){
		return(double(x) - double(y) == (z));
	}
	#endif
}
