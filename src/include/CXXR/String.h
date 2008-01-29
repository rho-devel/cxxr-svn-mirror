/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1999-2006   The R Development Core Team.
 *  Andrew Runnalls (C) 2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

/** @file String.h
 * Class CXXR::String and associated C interface.
 */

#ifndef CXXR_STRING_H
#define CXXR_STRING_H

#include "CXXR/VectorBase.h"

#ifdef __cplusplus

#include "CXXR/SEXP_downcast.hpp"

namespace CXXR {
    /** @brief RObject representing a character string.
     */
    class String : public VectorBase {
    public:
	/** @brief Create a string, leaving its contents
	 *         uninitialized. 
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 */
	String(size_t sz);

	/** @brief Character access.
	 * @param index Index of required character (counting from
	 *          zero).  No bounds checking is applied.
	 * @return Reference to the specified character.
	 */
	char& operator[](unsigned int index)
	{
	    return m_data[index];
	}

	/** @brief Read-only character access.
	 * @param index Index of required character (counting from
	 *          zero).  No bounds checking is applied.
	 * @return the specified character.
	 * @note For CXXR internal use only.
	 */
	char operator[](unsigned int index) const
	{
	    return m_data[index];
	}

	const char* c_str() const
	{
	    return m_data;
	}

	/**
	 * @return the name by which this type is known in R.
	 */
	static const char* staticTypeName()
	{
	    return "char";
	}

	// Virtual function of RObject:
	const char* typeName() const;
    private:
	// Declared private to ensure that Strings are
	// allocated only using 'new'.
	~String()
	{
	    if (m_data != m_short_string)
		Heap::deallocate(m_data, length() + 1);
	}
    private:
	// Max. strlen stored internally:
	static const size_t s_short_strlen = 7;
	char* m_data;  // pointer to the string's data block.

	// If there are fewer than s_short_strlen+1 chars in the
	// string (including the trailing null), it is stored here,
	// internally to the String object, rather than via a separate
	// allocation from CXXR::Heap.  We put this last, so that it
	// will be adjacent to any trailing redzone.
	char m_short_string[s_short_strlen + 1];

	// Not implemented yet.  Declared to prevent
	// compiler-generated versions:
	String(const String&);
	String& operator=(const String&);
    };
}  // namespace CXXR

extern "C" {

#endif /* __cplusplus */

/**
 * @param x \c pointer to a \c String .
 * @return \c pointer to character 0 \a x .
 * @note For R internal use only.  May be removed in future.
 */
#ifndef __cplusplus
char *CHAR_RW(SEXP x);
#else
inline char *CHAR_RW(SEXP x)
{
    return &(*CXXR::SEXP_downcast<CXXR::String>(x))[0];
}
#endif

/**
 * @param x \c const pointer to a \c String .
 * @return \c const pointer to character 0 \a x .
 */
#ifndef __cplusplus
const char *R_CHAR(const SEXP x);
#else
inline const char *R_CHAR(const SEXP x)
{
    return CXXR::SEXP_downcast<CXXR::String>(x)->c_str();
}
#endif

# define LATIN1_MASK (1<<2)

/**
 * @param x Pointer to a \c VectorBase representing a character string.
 * @return true iff \a x is marked as having LATIN1 encoding.
 */
#ifndef __cplusplus
Rboolean IS_LATIN1(const SEXP x);
#else
inline Rboolean IS_LATIN1(const SEXP x)
{
    return Rboolean(x->m_gpbits & LATIN1_MASK);
}
#endif

/**
 * @brief Set LATIN1 encoding.
 * @param x Pointer to a \c VectorBase representing a character string.
 */
#ifndef __cplusplus
void SET_LATIN1(SEXP x);
#else
inline void SET_LATIN1(SEXP x) {x->m_gpbits |= LATIN1_MASK;}
#endif

/**
 * @brief Unset LATIN1 encoding.
 * @param x Pointer to a \c VectorBase representing a character string.
 */
#ifndef __cplusplus
void UNSET_LATIN1(SEXP x);
#else
inline void UNSET_LATIN1(SEXP x) {x->m_gpbits &= ~LATIN1_MASK;}
#endif

# define UTF8_MASK (1<<3)

/**
 * @param x Pointer to a \c VectorBase representing a character string.
 * @return true iff \a x is marked as having UTF8 encoding.
 */
#ifndef __cplusplus
Rboolean IS_UTF8(const SEXP x);
#else
inline Rboolean IS_UTF8(const SEXP x)
{
    return Rboolean(x->m_gpbits & UTF8_MASK);
}
#endif

/**
 * @brief Set UTF8 encoding.
 * @param x Pointer to a \c VectorBase representing a character string.
 */
#ifndef __cplusplus
void SET_UTF8(SEXP x);
#else
inline void SET_UTF8(SEXP x) {x->m_gpbits |= UTF8_MASK;}
#endif

/**
 * @brief Unset UTF8 encoding.
 * @param x Pointer to a \c VectorBase representing a character string.
 */
#ifndef __cplusplus
void UNSET_UTF8(SEXP x);
#else
inline void UNSET_UTF8(SEXP x) {x->m_gpbits &= ~UTF8_MASK;}
#endif

/**
 * @brief Create a string object.
 *
 *  Allocate a string object.
 * @param length The length of the string to be created (excluding the
 *          trailing null byte).
 * @return Pointer to the created string.
 */
#ifndef __cplusplus
SEXP Rf_allocString(R_len_t length);
#else
inline SEXP Rf_allocString(R_len_t length)
{
    return new CXXR::String(length);
}
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* CXXR_STRING_H */