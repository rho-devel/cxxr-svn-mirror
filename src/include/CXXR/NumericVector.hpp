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

namespace CXXR{
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
		 * @brief Class \c CXXR::NumericVector::Power implements T x ^ T y .
		 */
		class Power{
		public:
			static T op(T l, T r)throw(RangeException);
		private:
			Power();
			static double R_pow(double x, double y);
		};
	friend class Power;
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
	
	//Note: specilisations defined in NumericVector.cpp
	
	//NA_value specilisations.
	//for integer, return minimum int.
	template<>
	int NumericVector<int,INTSXP>::NA_value();
	//Specilisation for double.
	template<>
	double NumericVector<double,REALSXP>::NA_value();


	//Subtract specilisations.
	//int
	template<>
	int NumericVector<int,INTSXP>::Subtract::op(int l, int r)throw(RangeException);
	//double
	template<>
	double NumericVector<double,REALSXP>::Subtract::op(double l, double r)throw(RangeException);
	//complex
	template<>
	Rcomplex NumericVector<Rcomplex,CPLXSXP>::Subtract::op(Rcomplex l,Rcomplex r)throw(RangeException);

	/**
	 * @brief CXXR::binary_op is a non-member function that performs operation on \c NumericVector from another
	 * 		\c NumericVector.
	 * @return a new NumericVector, which is the answer.
	 */
	template<typename T, SEXPTYPE ST, class operation>
	NumericVector<T, ST>* binary_op(const NumericVector<T, ST>* l, const NumericVector<T, ST>* r){
		//answer is as large as the longest vector.
		const unsigned int ansSize= (l->size() > r->size()) ? (l->size()) : (r->size());
		if(l->size()==0 || r->size()==0){
			return new NumericVector<T,ST>(0);
		}
		NumericVector<T,ST>* ans = new NumericVector<T,ST>(ansSize);
		bool overflowed=false;
		//either r will need to repeat...
		if(ansSize==l->size()){
			for(int i, i2=0; i<ansSize; i2=(++i2==r->size())?0:i2, ++i){
				try{
					(*ans)[i]=operation::op((*l)[i],(*r)[i2]);
				}catch(...){
					overflowed = true;
					(*ans)[i]=NumericVector<T,ST>::NA_value();
				}
			}
		}else{ //...or l will. Note: this is almost code duplication, but the alternative is to use a #define. (or a less efficient loop.)
			for(int i,i2=0; i<ansSize; i2=(++i2==l->size())?0:i2, ++i){
				try{
					(*ans)[i]=operation::op((*l)[i2],(*r)[i]);
				}catch(...){
					overflowed=true;
					(*ans)[i]=NumericVector<T,ST>::NA_value();
				}
			}
		}
		if(overflowed){
			//TODO:warning....hmmm
		}
		//note:attribute related operations required next.

		return ans;
	}


	//range-check function definitions.
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
}
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif
#endif /* NUMERICVECTOR_HPP */
