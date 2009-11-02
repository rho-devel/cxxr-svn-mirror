/**
 * @file NumericVector.h
 * @brief Class CXXR::NumericVector
 */

#ifndef NUMERICVECTOR_HPP
#define NUMERICVECTOR_HPP 1

#include "CXXR/DumbVector.hpp"
#include "Defn.h"
namespace CXXR{
	template <typename T, SEXPTYPE ST>
	class NumericVector : public DumbVector<T,ST>{
	public:
		/**
		 * @brief Constructs an uninitialized \c NumericVector of size sz.
		 * @param sz Size to make the \c NumericVector, may be zero.
		 */
		NumericVector(size_t sz);
		/**
		 * @brief Constructs an initialized \c NumericVector of size sz.
		 * @param sz Size to make the \c NumericVector, may be zero.
		 * @param initializer Every element of the \c NumericVector will be set to this value.
		 */
		NumericVector(size_t sz, const T& initializer);
		/**
		 * @brief Returns the value used internally to represent NA for the given instantiation of \c NumericVector .
		 * @return NA value for type T.
		 */
		static T NA_value();
		
		/**
		 * @brief Returns the internal type name used by a given instantiation of \c NumericVector .
		 * @return An internal type name.
		 */
		static const char* staticTypeName();
		/**
		 * @brief Class CXXR::NumericVector::Subtract implements a subtract operation between two \c NumericVector classes.
		 */
		class Subtract{
			public:
			Subtract();
			virtual ~Subtract();
			/**
			 * @brief op implements the subtraction operation between two values of type T.
			 * @param l the left \c value for the operation l-r.
			 * @param r the right \c value for the operation l-r.
			 * @return the result of l-r. If either l or r were NA, the result is NA. 
			 * 			If neither were NA, but the result is NA, an error has occured.
			 */
			static T op(T l, T r);
		};
	private:
		/**
		 * @brief Private deconstructor; Prevents creation on the stack.
		 */
		virtual ~NumericVector();
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
		//definitions used to perform range-checks on integers.
		static inline bool GOODISUM(int x, int y, int z);
		static inline bool GOODIPROD(int x,int y, int z);
		static inline bool OPPOSITE_SIGNS(int x, int y);
		static inline bool GOODIDIFF(int x, int y, int z);
	};
}
#endif
