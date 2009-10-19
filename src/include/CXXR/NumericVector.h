#ifndef NUMERICVECTOR_HPP
#define NUMERICVECTOR_HPP 1

#include "CXXR/DumbVector.hpp"
#include <limits>
namespace CXXR{
    template <typename T, SEXPTYPE ST>
    class NumericVector : public DumbVector<T,ST>{
        public:
            NumericVector();
            //! Returns NA value for the template type. See specilisations.
            static T NA_value();
	    
	    class Subtract{
	    	public:
		    Subtract();
		    ~Subtract();
		    static T op(T l, T r);
	    };
        private:
	    //! private deconstructor prevents creation on the stack.
            virtual ~NumericVector();
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
}
#endif
