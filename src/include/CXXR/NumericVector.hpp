#ifndef NUMERICVECTOR_HPP
#define NUMERICVECTOR_HPP 1

#include "CXXR/DumbVector.hpp"

namespace CXXR{
    template <typename T, SEXPTYPE ST>
    class NumericVector : public DumbVector {
        public:
            NumericVector()
                    : DumbVector<T,ST>()
            {
            }

            //! Returns the appropriate internal NA value for type T.
            static T NA_value()
            {
                //find values of:
                //NA_REAL
                //NA_INTEGER
                return 0; //temporary...
            }

        private:
            virtual ~NumericVector(){ }
    };
}

#endif
