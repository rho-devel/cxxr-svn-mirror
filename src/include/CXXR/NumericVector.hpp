#ifndef NUMERICVECTOR_HPP
#define NUMERICVECTOR_HPP 1

#include "CXXR/DumbVector.hpp"
#include <limits>
namespace CXXR{
    template <typename T, SEXPTYPE ST>
    class NumericVector : public DumbVector<T,ST>{
        public:
            NumericVector() : DumbVector<T,ST>(){}

            //! Returns NA value for the template type. See specilisations.
            static T NA_value(){return NULL; /*maybe throw an exception instad.*/ }
	    
	    class Subtract{
	    	public:
		    Subtract(){};
		    ~Subtract(){};
		    static T op(T l, T r){return NULL;}
	    };
        private:
	    //! private deconstructor prevents creation on the stack.
            virtual ~NumericVector(){ }
	    //! definitions used with real numbers.
	    typedef union
	    {
	        double value;
		unsigned int word[2];
	    } ieee_double;
	    #ifdef WORDS_BIGENDIAN
	    static const int ieee_hw = 0;
	    static const int ieee_lw = 1;
	    #else  /* !WORDS_BIGENDIAN */
	    static const int ieee_hw = 1;
	    static const int ieee_lw = 0;
	    #endif /* WORDS_BIGENDIAN */
    };
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

#endif
