/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-11 Andrew R. Runnalls, subject to such other
 *CXXR copyrights and copyright restrictions as may be stated below.
 *CXXR 
 *CXXR CXXR is not part of the R project, and bugs and other issues should
 *CXXR not be reported via r-bugs or other R project channels; instead refer
 *CXXR to the CXXR website.
 *CXXR */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1999-2006   The R Development Core Team.
 *  Andrew Runnalls (C) 2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

/** @file VectorBase.h
 * @brief Class CXXR::VectorBase and associated C interface.
 */

#ifndef VECTORBASE_H
#define VECTORBASE_H

#include "CXXR/RObject.h"

#ifdef __cplusplus

#include "CXXR/SEXP_downcast.hpp"

typedef CXXR::RObject VECTOR_SEXPREC, *VECSEXP;

namespace CXXR {
    class ListVector;
    class StringVector;

    template <typename T, SEXPTYPE ST> class DumbVector;
    typedef DumbVector<int, INTSXP> IntVector;

    /** @brief Value to be used if 'not available'.
     *
     * @tparam T type used as an element in the CXXR implementation of
     *           an R vector type.
     *
     * @return The value of this type to be used if the actual value
     * is not available.  This for example is the value that will be
     * used if an element of a vector of \a T is accessed in R using a
     * NA index.
     *
     * @note For some types, e.g. Rbyte, the value returned is not
     * distinct from ordinary values of the type.  See
     * hasDistinctNA().
     */
    template <typename T>
    const T& NA();

    /** @brief Does a value represent a distinct 'not available'
     *  status?
     *
     * @tparam T type used as an element in the CXXR implementation of
     *           an R vector type.
     *
     * @param t A value of type \a T .
     *
     * @return true iff \a t has a distinct value (or possibly, one of
     * a set of distinct values) signifying that the actual value of
     * this quantity is not available.
     */
    template <typename T>
    inline bool isNA(const T& t)
    {
	return t == NA<T>();
    }

    /** @brief Does a type have a distinct 'not available' value.
     *
     * @tparam T type used as an element in the CXXR implementation of
     *           an R vector type.
     *
     * @return true iff the range of type \a T includes a distinct
     * value to signify that the actual value of the quantity is not
     * available.
     */
    template <typename T>
    inline bool hasDistinctNA()
    {
	return isNA(NA<T>());
    }

    /** @brief Untemplated base class for R vectors.
     */
    class VectorBase : public RObject {
    public:
	/**
	 * @param stype The required ::SEXPTYPE.
	 * @param sz The required number of elements in the vector.
	 */
	VectorBase(SEXPTYPE stype, size_t sz)
	    : RObject(stype), m_truelength(sz), m_size(sz)
	{}

	/** @brief Copy constructor.
	 *
	 * @param pattern VectorBase to be copied.
	 */
	VectorBase(const VectorBase& pattern)
	    : RObject(pattern), m_truelength(pattern.m_truelength),
	      m_size(pattern.m_size)
	{}

	/** @brief Names associated with the rows, columns or other
	 *  dimensions of an R matrix or array.
	 *
	 * @return A null pointer signifies that there are no names
	 * associated with any of the dimensions of \a *this ; a null
	 * pointer will always be returned if \a *this is not an R
	 * matrix or array.  Otherwise the return value will be a
	 * pointer to ListVector with as many elements as \a *this has
	 * dimensions.  Each element of the ListVector is either a
	 * null pointer, signifying that there are no names associated
	 * with the corresponding dimension, or a pointer to a
	 * StringVector with as many elements as the size of the array
	 * along the corresponding dimension, giving the names of the
	 * 'slices' along that dimension.  For example the zeroth
	 * element of the ListVector, if non-null, will give the row
	 * names, and the following element will give the column
	 * names.
	 */
	const ListVector* dimensionNames() const;

	/** @brief Names associated with a particular dimension of an
	 *  R matrix or array.
	 *
	 * @param d Dimension number (counting from 1) for which
	 *          dimension names are required.  Must be non-zero (not
	 *          checked).
	 *
	 * @return A null pointer if no names are associated with
	 * dimension \a d of \a *this , if \a *this does not have as
	 * many as \a d dimensions, or if \a *this is not an R matrix
	 * or array.  Otherwise a pointer to a StringVector with as
	 * many elements as the size of \a *this along the
	 * corresponding dimension, giving the names of the 'slices'
	 * along that dimension.
	 */
	const StringVector* dimensionNames(unsigned int d) const;

	/** @brief Dimensions of R matrix or array.
	 *
	 * @return A null pointer if \a *this is not an R matrix or
	 * array.  Otherwise a pointer to an IntVector with one or
	 * more elements, the product of the elements being equal to
	 * the number of elements in \a *this .  The number of
	 * elements in the return value is the dimensionality of the
	 * array (e.g. 2 for a matrix), and each element gives the
	 * size of the array along the respective dimension.
	 */
	const IntVector* dimensions() const;

	/** @brief Names of vector elements.
	 *
	 * @return either a null pointer (if the elements do not have
	 * names), or a pointer to a StringVector with the same number
	 * of elements as \a *this .
	 */
	const StringVector* names() const;

	/** @brief Associate names with the rows, columns or other
	 *  dimensions of an R matrix or array.
	 *
	 * @param names If this is a null pointer, any names currently
	 *          associated with the dimensions of \a *this will be
	 *          removed.  Otherwise \a names must be a pointer to
	 *          ListVector with as many elements as \a *this has
	 *          dimensions.  Each element of the ListVector must
	 *          be either a null pointer, signifying that no names
	 *          are to be associated with the corresponding
	 *          dimension, or a pointer to a StringVector with as
	 *          many elements as the size of the array along the
	 *          corresponding dimension, giving the names to be
	 *          given to the 'slices' along that dimension.  For
	 *          example the zeroth element of the ListVector, if
	 *          non-null, will give the row names, and the
	 *          following element will give the column names.  \a
	 *          *this will assume ownership of this ListVector
	 *          (rather than duplicating it), so the calling code
	 *          must not subsequently modify it.
	 */
	void setDimensionNames(ListVector* names);

	/** @brief Associate names with a particular dimension of an
	 *  R matrix or array.
	 *
	 * @param d Dimension number (counting from 1) with which
	 *          dimension names are to be associated.  Must not be
	 *          greater than the * number of dimensions of \a
	 *          *this (checked).
	 *
	 * @param names If this is a null pointer, any names currently
	 *          associated with dimension \a d of \a *this are
	 *          removed.  Otherwise \a names must be a pointer to
	 *          a StringVector with as many elements as the size
	 *          of \a *this along the corresponding dimension,
	 *          giving the names to be given to the 'slices' along
	 *          that dimension.  \a *this will assume ownership of
	 *          this StringVector (rather than duplicating it), so
	 *          the calling code must not subsequently modify it.
	 */
	void setDimensionNames(unsigned int d, StringVector* names);

	/** @brief Define the dimensions of R matrix or array.
	 *
	 * As a side-effect, this function will remove any dimension names.
	 *
	 * @param dims If this is a null pointer, any existing dimensions
	 *          associated will be removed, and \a *this will
	 *          cease to be a R matrix/array.  Otherwise \a dims
	 *          must be a pointer to an IntVector with one or more
	 *          elements, the product of the elements being equal
	 *          to the number of elements in \a *this . The number
	 *          of elements in \a dims is the required
	 *          dimensionality of the array (e.g. 2 for a matrix),
	 *          and each element gives the required size of the
	 *          array along the respective dimension.  \a *this
	 *          will assume ownership of this IntVector (rather
	 *          than duplicating it), so the calling code must not
	 *          subsequently modify it.
	 */
	void setDimensions(IntVector* dims);

	/** @brief Associate names with the elements of a VectorBase.
	 *
	 * @param names Either a null pointer, in which case any
	 * existing names will be removed, or a pointer to a
	 * StringVector with the same number of elements as \a *this .
	 * \a *this will assume ownership of this StringVector (rather
	 * than duplicating it), so the calling code must not
	 * subsequently modify it.
	 */
	void setNames(StringVector* names);

	/** @brief Reduce the number of elements in the vector.
	 *
	 * @param new_size New size required.  Zero is permissible,
	 *          but the new size must not be greater than the
	 *          current size.
	 *
	 * @deprecated May be withdrawn in future.  Currently used by
	 * SETLENGTH() (which itself is little used).
	 */
	void shrink(size_t new_size);

	/** @brief Number of elements in the vector.
	 *
	 * @return The number of elements in the vector.
	 */
	size_t size() const
	{
	    return m_size;
	}

	/** @brief The name by which this type is known in R.
	 *
	 * @return the name by which this type is known in R.
	 */
	static const char* staticTypeName()
	{
	    return "(vector type)";
	}

	// Make private in due course (or get rid altogether):
	R_len_t m_truelength;
    protected:
	~VectorBase() {}
    private:
	size_t m_size;
    };
}  // namespace CXXR

extern "C" {

#endif /* __cplusplus */

/* Accessor functions */

/* Vector Access Functions */

/**
 * @param x Pointer to an CXXR::VectorBase .
 *
 * @return The length of \a x, or 0 if \a x is a null pointer.  (In 
 *         the case of certain hash tables, this means the 'capacity'
 *         of \a x , not all of which may be used.)
 */
#ifndef __cplusplus
int LENGTH(SEXP x);
#else
inline int LENGTH(SEXP x)
{
    using namespace CXXR;
    if (!x) return 0;
    VectorBase& vb = *SEXP_downcast<VectorBase*>(x);
    return vb.size();
}
#endif

/**
 * @param x Pointer to a CXXR::VectorBase .
 *
 * @return The 'true length' of \a x.  According to the R Internals
 *         document for R 2.4.1, this is only used for certain hash
 *         tables, and signifies the number of used slots in the
 *         table.
 */
#ifndef __cplusplus
int TRUELENGTH(SEXP x);
#else
inline int TRUELENGTH(SEXP x)
{
    using namespace CXXR;
    VectorBase& vb = *SEXP_downcast<VectorBase*>(x);
    return vb.m_truelength;
}
#endif

/**
 * Set length of vector.
 *
 * @param x Pointer to a CXXR::VectorBase .
 *
 * @param v The required new length, which must not be greater than
 *          the current length.
 *
 * @deprecated May be withdrawn in future.  Currently used in
 * library/stats/src/isoreg.c .
 */
void SETLENGTH(SEXP x, int v);

/**
 * Set 'true length' of vector.
 * @param x Pointer to a CXXR::VectorBase .
 * @param v The required new 'true length'.
 */
#ifndef __cplusplus
void SET_TRUELENGTH(SEXP x, int v);
#else
inline void SET_TRUELENGTH(SEXP x, int v)
{
    using namespace CXXR;
    VectorBase& vb = *SEXP_downcast<VectorBase*>(x);
    vb.m_truelength = v;
}
#endif

/**
 * @brief Create a vector object.
 *
 *  Allocate a vector object.  This ensures only validity of
 *  ::SEXPTYPE values representing lists (as the elements must be
 *  initialized).  Initializing of other vector types is done in
 *  do_makevector().
 *
 * @param stype The type of vector required.
 *
 * @param length The length of the vector to be created.
 *
 * @return Pointer to the created vector.
 */
SEXP Rf_allocVector(SEXPTYPE stype, R_len_t length);

/** @brief Is an RObject a vector?
 *
 * Vector in this context embraces R matrices and arrays.
 *
 * @param s Pointer to the RObject to be tested.  The pointer may be
 *          null, in which case the function returns FALSE.
 *
 * @return TRUE iff \a s points to a vector object.
 */
Rboolean Rf_isVector(SEXP s);

#ifdef __cplusplus
}
#endif

#endif /* VECTORBASE_H */
