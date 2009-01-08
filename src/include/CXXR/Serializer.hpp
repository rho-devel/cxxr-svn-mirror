#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#ifdef __cplusplus

#include <iostream>
#include "CXXR/RObject.h"
#include "CXXR/Streams.h"

namespace CXXR {
	/** @brief Serializer
	 *
	 * At the beginning of serialization, a hash table is created
	 * to avoid duplicating objects. At the moment, this is initialized
	 * by the R_Serialize function and a reference stored as a class variable
	 * here. However, there is no reason why this could not be later
	 * initialised in the constuctor, once all serialization behaviour has 
	 * eventually been moved away from serialize.cpp
	 *
	 * This class contains accessor methods for fetching the R_outpstream_t
	 * and hash table SEXP.
	 */
	class Serializer {
	public:
		Serializer(R_outpstream_t st, SEXP ht)
			: s_stream(st), s_hashtable(ht) { }

		/* @brief Hash Table accessor method
		 */
		SEXP hashtable() const {
			return s_hashtable;
		}

		/* @brief Stream accessor method
		 */
		R_outpstream_t stream() const {
			return s_stream;
		}

	private:
		R_outpstream_t s_stream;
		SEXP s_hashtable;
	};
} // Namespace CXXR

#endif // __cplusplus

#endif // SERIALIZER_HPP

