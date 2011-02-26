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

/** @file Subscripting.hpp
 *
 * @brief Functionality to support subscript operations on R vectors,
 * matrices and arrays.
 */

#ifndef SUBSCRIPTING_HPP
#define SUBSCRIPTING_HPP 1

#include "CXXR/GCStackRoot.hpp"
#include "CXXR/IntVector.h"
#include "CXXR/ListVector.h"
#include "CXXR/PairList.h"
#include "CXXR/StringVector.h"
#include "CXXR/Symbol.h"

namespace CXXR {
    /** @brief Names associated with the rows, columns or other
     *  dimensions of an R matrix or array.
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          dimension names are required.
     *
     * @return A null pointer signifies that there are no names
     * associated with any of the dimensions of \a v ; a null pointer
     * will always be returned if \a v is not an R matrix or array.
     * Otherwise the return value will be a pointer to ListVector with
     * as many elements as \a v has dimensions.  Each element of the
     * ListVector is either a null pointer, signifying that there are
     * no names associated with the corresponding dimension, or a
     * pointer to an object inheriting from VectorBase with as many
     * elements as the size of the array along the corresponding
     * dimension, giving the names to be given to the 'slices' along
     * that dimension.  For example the zeroth element of the
     * ListVector, if non-null, will give the row names, and the
     * following element will give the column names.
     */
    inline const ListVector* dimensionNames(const VectorBase* v)
    {
	return static_cast<const ListVector*>(v->getAttribute(DimNamesSymbol));
    }

    /** @brief Names associated with a particular dimension of an
     *  R matrix or array.
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          dimension names are required.
     *
     * @param d Dimension number (counting from 1) for which
     *          dimension names are required.  Must be non-zero (not
     *          checked).
     *
     * @return A null pointer if no names are associated with
     * dimension \a d of \a v , if \a v does not have as many as \a d
     * dimensions, or if \a v is not an R matrix or array.  Otherwise
     * a pointer to an object inheriting from VectorBase with as many
     * elements as the size of \a v along the corresponding dimension,
     * giving the names to be given to the 'slices' along that
     * dimension.
     */
    const VectorBase* dimensionNames(const VectorBase* v, unsigned int d);

    /** @brief Dimensions of R matrix or array.
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          dimensions are required.
     *
     * @return A null pointer if \a v is not an R matrix or array.
     * Otherwise a pointer to an IntVector with one or more elements,
     * the product of the elements being equal to the number of
     * elements in the vector.  The number of elements is the
     * dimensionality of the array (e.g. 2 for a matrix), and each
     * element gives the size of the array along the respective
     * dimension.
     */
    inline const IntVector* dimensions(const VectorBase* v)
    {
	return static_cast<const IntVector*>(v->getAttribute(DimSymbol));
    }

    /** @brief Names of vector elements.
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          element names are required.
     *
     * @return either a null pointer (if the elements do not have
     * names), or a pointer to a StringVector with the same number
     * of elements as \a v .
     */
    inline const StringVector* names(const VectorBase* v)
    {
	return static_cast<const StringVector*>(v->getAttribute(NamesSymbol));
    }

    /** @brief Associate names with the rows, columns or other
     *  dimensions of an R matrix or array.
     *
     * @param v Non-null pointer to the VectorBase object with which
     *          dimension names are to be associated.
     *
     * @param names If this is a null pointer, any names currently
     *          associated with the dimensions of \a v will be
     *          removed.  Otherwise \a names must be a pointer to
     *          ListVector with as many elements as \a v has
     *          dimensions.  Each element of the ListVector must be
     *          either a null pointer, signifying that no names are to
     *          be associated with the corresponding dimension, or a
     *          pointer to an object inheriting from VectorBase with
     *          as many elements as the size of the array along the
     *          corresponding dimension, giving the names to be given
     *          to the 'slices' along that dimension.  For example the
     *          zeroth element of the ListVector, if non-null, will
     *          give the row names, and the following element will
     *          give the column names.  The VectorBase object pointed
     *          to by \a v will assume ownership of this IntVector
     *          (rather than duplicatingit), so the calling code must
     *          not subsequently modify it.
     */
    inline void setDimensionNames(VectorBase* v, ListVector* names)
    {
	v->setAttribute(DimNamesSymbol, names);
    }

    /** @brief Associate names with a particular dimension of an
     *  R matrix or array.
     *
     * @param v Non-null pointer to the VectorBase object with which
     *          dimension names are to be associated. 
     *
     * @param d Dimension number (counting from 1) with which
     * dimension names are to be associated.  Must not be greater than the
     * number of dimensions of \a v (checked). 
     *
     * @param names If this is a null pointer, any names currently
     *          associated with dimension \a d of \a v are removed.
     *          Otherwise \a names must be a pointer to an object
     *          inheriting from VectorBase with as many elements as
     *          the size of \a v along the corresponding dimension,
     *          giving the names to be given to the 'slices' along
     *          that dimension.  The VectorBase object pointed to by
     *          \a v will assume ownership of this StringVector
     *          (rather than duplicating it), so the calling code must
     *          not subsequently modify it.
     */
    void setDimensionNames(VectorBase* v, unsigned int d, VectorBase* names);

    /** @brief Define the dimensions of R matrix or array.
     *
     * As a side-effect, this function will remove any dimension names
     * associated with \a v .
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          dimensions are to be set.
     *
     * @param dims If this is a null pointer, any existing dimensions
     *          associated with \a v will be removed, and \a v will
     *          cease to be a R matrix/array.  Otherwise \a dims must
     *          be a pointer to an IntVector with one or more
     *          elements, the product of the elements being equal to
     *          the number of elements in the vector. The number of
     *          elements is the required dimensionality of the array
     *          (e.g. 2 for a matrix), and each element gives the
     *          required size of the array along the respective
     *          dimension.  The VectorBase object pointed to by \a v
     *          will assume ownership of this IntVector (rather than
     *          duplicating it), so the callingcode must not
     *          subsequently modify it.
     */
    inline void setDimensions(VectorBase* v, IntVector* dims)
    {
	v->setAttribute(DimSymbol, dims);
    }

    /** @brief Associate names with the elements of a VectorBase.
     *
     * @param v Non-null pointer to the VectorBase object whose
     *          element names are to be set.
     *
     * @param names Either a null pointer, in which case any existing
     * names will be removed, or a pointer to a StringVector with the
     * same number of elements as \a v .  The VectorBase object
     * pointed to by \a v will assume ownership of this StringVector
     * (rather than duplicating it), so the calling code must not
     * subsequently modify it.
     */
    inline void setNames(VectorBase* v, StringVector* names)
    {
	v->setAttribute(NamesSymbol, names);
    }

    /** @brief Services to support R subscripting operations.
     *
     * This class, all of whose members are static, encapsulates
     * services supporting subscripting of R vector objects, including
     * R matrices and arrays.
     */
    class Subscripting {
    public:
	template <class V>
	static V* arraySubset(const V* v, const PairList* indices)
	{
	    const IntVector* vdims = dimensions(v);
	    size_t ndims = vdims->size();
	    size_t resultsize = 1;
	    std::vector<DimIndexer> dimindexer(ndims);
	    // Set up the DimIndexer objects:
	    {
		const PairList* pl = indices;
		for (unsigned int d = 0; d < ndims; ++d) {
		    DimIndexer& di = dimindexer[d];
		    const IntVector* iv = static_cast<IntVector*>(pl->car());
		    di.nindices = iv->size();
		    resultsize *= di.nindices;
		    di.indices = &(*iv)[0];
		    pl = pl->tail();
		}
		dimindexer[0].stride = 1;
		for (unsigned int d = 1; d < ndims; ++d)
		    dimindexer[d].stride
			= dimindexer[d - 1].stride * (*vdims)[d - 1];
	    }
	    GCStackRoot<V> result(CXXR_NEW(V(resultsize)));
	    // Copy elements across:
	    {
		// ***** FIXME *****  Currently needed because Handle's
		// assignment operator takes a non-const RHS:
		V* vnc = const_cast<V*>(v);
		std::vector<unsigned int> indexnum(ndims, 0);
		for (unsigned int iout = 0; iout < resultsize; ++iout) {
		    bool naindex = false;
		    unsigned int iin = 0;
		    for (unsigned int d = 0; d < ndims; ++d) {
			const DimIndexer& di = dimindexer[d];
			int index = di.indices[indexnum[d]];
			if (index == NA_INTEGER) {
			    naindex = true;
			    break;
			}
			if (index < 1 || index > (*vdims)[d])
			    Rf_error(_("subscript out of bounds"));
			iin += (index - 1)*di.stride;
		    }
		    if (naindex)
			result->setNA(iout);
		    else (*result)[iout] = (*vnc)[iin];
		    // Advance index selection:
		    {
			unsigned int d = 0;
			while (d < ndims
			       && ++indexnum[d] >= dimindexer[d].nindices) {
			    indexnum[d] = 0;
			    ++d;
			}
		    }
		}
	    }
	    return result;
	}
		    
	/** @brief Extract a subset of an R vector object.
	 *
	 * @tparam V A type inheriting from VectorBase.
	 *
	 * @param v Non-null pointer to a \a V object, which is the
	 *          object from which a subset (not necessarily a
	 *          proper subset) is to be extracted.
	 *
	 * @param indices Pointer to a vector of indices (counting 1)
	 *          designating the elements of \a v to be included as
	 *          successive elements of the output vector, which
	 *          will be the same size as \a indices .  NA_INTEGER
	 *          is a permissible element value, in which case the
	 *          corresponding element of the output vector will
	 *          have an NA value appropriate to type \a V .  If an
	 *          index is out of range with respect to \a v , in
	 *          which case also the corresponding element of the
	 *          output vector will have an NA value appropriate to
	 *          type \a V .
	 *
	 * @return Pointer to a newly created object of type \a V ,
	 * containing the designated subset of \a v .
	 */
	template <class V>
	static V* vectorSubset(const V* v, const IntVector* indices)
	{
	    size_t ni = indices->size();
	    GCStackRoot<V> ans(CXXR_NEW(V(ni)));
	    size_t vsize = v->size();
	    // ***** FIXME *****  Currently needed because Handle's
	    // assignment operator takes a non-const RHS:
	    V* vnc = const_cast<V*>(v);
	    for (unsigned int i = 0; i < ni; ++i) {
		int ii = (*indices)[i];
		// Note that zero and negative indices ought not to occur.
		if (ii == NA_INTEGER || ii <= 0 || ii > int(vsize))
		    ans->setNA(i);
		else (*ans)[i] = (*vnc)[ii - 1];
	    }
	    setVectorAttributes(ans, v, indices);
	    return ans;
	}
    private:
	// Data structure used in subsetting arrays, containing
	// information relating to a particular dimension. 
	struct DimIndexer {
	    unsigned int nindices;  // Number of index values to be
	      // extracted along this dimension.
	    const int* indices;  // Pointer to array containing the index
	      // values themselves.  The index values count from 1. 
	    unsigned int stride;  // Number of elements (within the
	      // linear layout of the source array) separating
	      // consecutive elements along this dimension.
	};

	// Not implemented.  Declared private to prevent the
	// inadvertent creation of Subscripting objects.
	Subscripting();

	/** @brief Set the attributes on a vector subset.
	 *
	 * This function sets up the 'names' and 'srcref' attributes
	 * on an R vector obtained by subsetting, by applying the
	 * corresponding subsetting to the 'names' and 'srcref's
	 * attributes of the source object.  If the source object has
	 * no 'names' attribute, but is a one-dimensional array, then
	 * the dimension names ('row names'), if any, associated with
	 * that one dimension, are used instead.
	 *
	 * @param subset Non-null pointer to an R vector representing
	 *          a subset of \a source defined by \a indices .
	 *
	 * @param source Non-null pointer to the object of which \a
	 *          subset is a subset.
	 *
	 * @param indices Non-null pointer to the index vector used to
	 *          form \a subset from \a source .
	 */
	static void setVectorAttributes(VectorBase* subset,
					const VectorBase* source,
					const IntVector* indices);
    };
}

#endif  // SUBSCRIPTING_HPP
