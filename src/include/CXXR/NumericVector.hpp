/**
 * @file NumericVector.hpp
 * @brief Template Class CXXR::NumericVector
 */

#ifndef NUMERICVECTOR_HPP
#define NUMERICVECTOR_HPP 1

#ifdef __cplusplus

#include <exception>
#include "CXXR/DumbVector.hpp"
#include "R_ext/Complex.h"
#include "CXXR/GCStackRoot.h"
#include "CXXR/errors.h"
#include <limits>
#include <cmath>
#include <R_ext/Arith.h>

/*FIXME these following definitions required as: 
 * 	1> there is no coerce.h
 * 	2> Defn.h acts as a coerce.h (and many other things.) But this file is::
 * 		a) A taboo for new CXXR code, so including it in a header is a major taboo.
 * 		b) currently not possible to include it here. Doing so will cause failure.
*/
	extern "C" {SEXP Rf_coerceVector(SEXP, SEXPTYPE);}
/* end of FIXME section */
namespace CXXR{
	/** @brief Vector of numeric data.
	 *
	 */
	template <typename T, SEXPTYPE ST>
	class NumericVector : public DumbVector<T,ST>{
	public:
		/**
		 * Clone is a virtual function of RObject.
		 */
		NumericVector<T,ST >* clone() const{
			return expose(new NumericVector<T,ST>(*this));
		}

		/**
		 * @brief Constructs an uninitialized \c NumericVector of size sz.
		 * @param sz Size to make the \c NumericVector, may be zero.
		 */
		NumericVector(size_t sz):
				DumbVector<T,ST>(sz){
		}
		/**
		 * @brief Constructs an initialized \c NumericVector of size sz.
		 * @param sz Size to make the \c NumericVector, may be zero.
		 * @param initializer Every element of the \c NumericVector will be set to this value.
		 */
		NumericVector(size_t sz, const T& initializer):
				DumbVector<T,ST>(sz,initializer){
		}

		/**
		 * @brief Class \c CXXR::NumericVector::rangeException may be thrown by NumericVector when a range error has occured.
		 */
		class RangeException : public std::exception{
			/**
			 * An explanation for the exception, although this is unlikely to actually be called.
			 */
			virtual const char* what() const throw(){
				return "A range error has occured.";
			}
		};
		
		/**
		 * @breif Returns the value used internally to represent NA for the given instantiation of \c NumericVector .
		 * @return NA value for type T.
		 */
		static T NA_value();
		
		/**
		 * @brief Class \c CXXR::NumericVector::Subtract implements a subtract operation between two values of type T .
		 */
		class Subtract{
		public:
			/**
			 * @brief op implements the subtraction operation between two values of type T .
			 * @param l the left \c value for the operation l-r.
			 * @param r the right \c value for the operation l-r.
			 * @return the result of l-r. If either l or r were NA, the result is NA. 
			 * 			If neither were NA, but the result is NA, an error has occured.
			 */
			static T op(T l, T r)throw(RangeException); //specalised implementations only.
		private:
			/**
			 * Private constructor as this class only provides a static function
			 */
			Subtract();
			static inline bool goodIDiff(int x, int y, int z);
			static inline bool oppositeSigns(int x, int y){
				return (x < 0)^(y < 0);
			}
		};

		/** 
		 * @brief Class \c CXXR::NumericVector::Add implements an addition of two values of type T .
		 */
		class Add{
		public:
			/**
			 *	@brief op implements the addition of two vlaues of type T .
			 */
			static T op(T l, T r)throw(RangeException);
		private:
			Add();
			static inline bool goodISum(int x, int y, int z);
		};

		/**
		 * @brief Class \c CXXR::NumericVector::Divide implements a devision of two values of type T .
		 */
		class Divide{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			Divide();
		};
		/**
		 * @brief Class \c CXXR::NumericVector::IDivide provides 'integer division'.
		 */
		class IDivide{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			IDivide();
		};

		/**
		 * @brief Class \c CXXR::NumericVector::Multiply implements multiplication of two values of type T .
		 */
		class Multiply{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			Multiply();
			static inline bool goodIProd(int x,int y, int z){
				return double(x) * double(y) == z;
			}
		};
		
		/**
		 * @brief Class \c CXXR::NumericVector::Power implements l ^ r .
		 */
		class Power{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			Power();
			static double myfmod(double l, double r);
			static double R_pow(double x, double y);
		};

		/**
		 * @brief Class \c CXXR::NumericVector::Modulo provides us with l % r
		 */
		class Modulo{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			Modulo();
			static double myfmod(double l, double r);
		};

	private:
		/**
		 * @brief Private deconstructor; Prevents creation on the stack.
		 */
		~NumericVector(){
		}
		static double myfmod(double x,double y);
		//definitions used to manipulate real numbers in an endian-safe way.
		typedef union{
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


	/**
	 * @brief CXXR::binary_op performs binary aritmetic operations
	 * Notes: lp and rp will be coerced into the type ST if it's
	 * possible to do so and if they are not already of that type.
	 * @param lp The left robject for the operation.
	 * @param rp The right robject for the operation.
	 * @return A new NumericVector<T,ST> which contains the answer.
	 */
	template<typename T, SEXPTYPE ST, class operation>
	inline NumericVector<T, ST>* binary_op(RObject* lp, RObject* rp){
		GCStackRoot<NumericVector<T,ST> > l,r;
		l=SEXP_downcast<NumericVector<T,ST>* >(Rf_coerceVector(lp,ST));
		r=SEXP_downcast<NumericVector<T,ST>* >(Rf_coerceVector(rp,ST));
		//answer is large as longest vector, or 0.
		const unsigned int ansSize= (l->size() > r->size()) ? (l->size()) : (r->size());
		if(l->size()==0 || r->size()==0){
			GCStackRoot<NumericVector<T,ST> > ans(GCNode::expose(new NumericVector<T,ST>(0)));
			return ans;
		}
		GCStackRoot<NumericVector<T,ST> > ans(GCNode::expose(new NumericVector<T,ST>(ansSize)));
		bool overflowed=false;
		// binary operation main loop
		for(unsigned int i=0; i< ansSize; i++){
			try{
				(*ans)[i]=operation::op((*l)[ i % l->size() ],(*r)[ i % r->size() ]);
			}catch(...){
				overflowed = true;
				(*ans)[i]=NumericVector<T,ST>::NA_value();
			}
		}
		//warn about overflow error in the usual way.
		if(overflowed){
			Rf_warningcall(0, _("NAs produced by integer overflow"));
		}

		//Copy attributes, if required.
		if(l->attributes() == 0 && r->attributes() == 0)
			return ans;

		if(l->size() > r->size()){
			Rf_copyMostAttrib(l, ans);
		}else if (l->size() == r->size()){
			Rf_copyMostAttrib(r, ans);
			Rf_copyMostAttrib(l, ans);
		}else{
			Rf_copyMostAttrib(r, ans);
		}
		return ans;
	}
	

	//NOTE: things were done this way originally in
	//arithmetic.cpp
	//
	//If USES_TWOS_COMPLEMENT can be discovered by the
	//configure script, it should be done so, then this define removed.
	//(and the one in arithmetic.cpp too)
	#ifndef USES_TWOS_COMPLEMENT
	#define USES_TWOS_COMPLEMENT
	#endif

	#ifdef USES_TWOS_COMPLEMENT
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::Add::goodISum(int x, int y, int z){
		return ((x > 0) ? (y < z) : ! (y < z));
	}
	template <typename T, SEXPTYPE ST>
	bool NumericVector<T,ST>::Subtract::goodIDiff(int x, int y, int z){
		return !(oppositeSigns(x, y) && oppositeSigns(x, z));
	}
	#else
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::Add::goodISum(x, y, z){
		return(double(x) + double(y) == (z));
	}
	template <typename T, SEXPTYPE ST>
	NumericVector<T,ST>::Subtract::goodIDiff(x, y, z){
		return(double(x) - double(y) == (z));
	}
	#endif /* USES_TWOS_COMPLEMENT */


//NA_value specilisations.
//for integer, return minimum int.
template<>
int inline NumericVector<int,INTSXP>::NA_value(){
	return std::numeric_limits<int>::min();
}
//Specilisation for double. Note: this creates and returns NaN.
template<>
double inline NumericVector<double,REALSXP>::NA_value(){
	volatile ieee_double x;	//volatile to avoid an obscure bug on some compiler...
	x.word[ieee_hw] = 0x7ff00000;
	x.word[ieee_lw] = 1954;
	return x.value;
}

//Subtract specilisations.
//int
template<>
int inline NumericVector<int,INTSXP>::Subtract::op(int l, int r)throw(RangeException){
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
double inline NumericVector<double,REALSXP>::Subtract::op(double l, double r)throw(RangeException){
	return (l-r);
}
//complex
template<>
Rcomplex inline NumericVector<Rcomplex,CPLXSXP>::Subtract::op(Rcomplex l,Rcomplex r)throw(RangeException){
	l.r -= (r.r);
	l.i -= (r.i);
	return l;
}
//Add specilisation
//int
template<>
int inline NumericVector<int, INTSXP>::Add::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value()){
		return NA_value();
	}
	int result=(l+r);
	if(result == NumericVector<int, INTSXP>::NA_value() || !goodISum(l,r,result)){
		RangeException rangeException;
		throw rangeException;
	}
	return result;
}
//double
template<>
double inline NumericVector<double,REALSXP>::Add::op(double l, double r)throw(RangeException){
	return (l+r);
}
//complex
template<>
Rcomplex inline NumericVector<Rcomplex,CPLXSXP>::Add::op(Rcomplex l, Rcomplex r)throw(RangeException){
	l.r += r.r;
	l.i += r.i;
	return l;
}
//Divide specilisations
//there is no int version.
//real
template<>
double inline NumericVector<double, REALSXP>::Divide::op(double l, double r)throw(RangeException){
	return l/r;
}
//iDiv int
template<>
int inline NumericVector<int, INTSXP>::IDivide::op(int l, int r)throw(RangeException){
	if(l==NA_value() || r==NA_value()){
		return NA_value();
	}
	if(r==0){return 0;}
	return int(floor(double(l)/double(r)));
}
//complex
// TODO - see complex.c

//multiply
//int
template<>
int inline NumericVector<int, INTSXP>::Multiply::op(int l, int r)throw(RangeException){
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
double inline NumericVector<double, REALSXP>::Multiply::op(double l, double r)throw(RangeException){
	return l*r;
}



}
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif
#endif /* NUMERICVECTOR_HPP */
