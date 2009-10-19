#include "CXXR/NumericVector.h"

namespace CXXR{
    //constructor/deconstructor
    template <typename T, SEXPTYPE ST>
    NumericVector<T,ST>::NumericVector():
            DumbVector<T,ST>(){
        
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

    //! Specilisation for int.
    template<>
    int NumericVector<int,INTSXP>::NA_value(){
    	return INT_MIN;
    }
    
    //! Specilisation for double.
    template<>
    double NumericVector<double,REALSXP>::NA_value(){
	volatile ieee_double x;
	x.word[ieee_hw] = 0x7ff00000;
        x.word[ieee_lw] = 1954;
	return x.value;
    }

    //! Naive subtract specilisation for int.
    template<>
    int NumericVector<int,INTSXP>::Subtract::op(int l, int r){
	// TODO: handling for result becoming NA_value.
	//test for NA.
	if(l==NumericVector<int,INTSXP>::NA_value() ||
		r==NumericVector<int,INTSXP>::NA_value()){
            return NumericVector<int,INTSXP>::NA_value();
	}
	int result = l-r;
	//handle underflow.
	if(result<=l){
	    return result;
	}else{
	    //TODO: figure best course of action.
	    return 0;
	}
    }

    //! Naive subtract specilisation for double.
    template<>
    double NumericVector<double,REALSXP>::Subtract::op(double l, double r){
	//TODO: handling for range.
	//test for NA.
	if(l==NumericVector<double,REALSXP>::NA_value() ||
	        r==NumericVector<double,REALSXP>::NA_value()){
	    return NumericVector<double,REALSXP>::NA_value();
	}
	double result = l-r;
	//handle underflow.
	if(result<=l){
	    return result;
	}else{
	    //TODO: figure best course of action.
	    return 0;
	}
    }
}
