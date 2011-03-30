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
   /** @brief Services to support R subscripting operations.
     *
     * This class, all of whose members are static, encapsulates
     * services supporting subscripting of R vector objects, including
     * R matrices and arrays.
     *
     * @par Canonical index vectors:
     * A canonical index vector is an IntVector in which each element
     * is either NA or a strictly positive integer (representing an
     * index into a vector counting from one).  Various
     * <tt>canonicalize()</tt> functions are provided to convert other
     * forms of subscripting to the canonical form.
     *
     * @note Under certain circumstances, a canonical index vector may
     * have a <tt>use.names</tt> attribute.  This attribute should be
     * regarded as private to the Subscripting class, because its
     * implementation (or indeed its use at all) may change in the
     * future.
     *
     * @todo A matter for possible review is the fact that during
     * subsetting and subassignment operations, CXXR sometimes makes
     * deep copies of the elements of HandleVector objects in cases
     * where CR gets away with simple pointer copies.
     */
    class Subscripting {
    public:
	/** @brief Extract a subset from an R matrix or array.
	 *
	 * @tparam V A type inheriting from VectorBase.
	 *
	 * @param v Non-null pointer to a \a V object, which is an R
	 *          matrix or array from which a subset (not
	 *          necessarily a proper subset) is to be extracted.
	 *
	 * @param indices Pointer to a PairList with as many elements
	 *          as \a v has dimensions.  Each element (car) of the
	 *          PairList is an IntVector giving the index values
	 *          (counting from 1) to be selected for the
	 *          corresponding dimension.  NA_INTEGER is a
	 *          permissible index value, in which case any
	 *          corresponding elements of the output array will
	 *          have an NA valueappropriate to type \a V .
	 *          Otherwise, all indices must be in range for the
	 *          relevant dimension of \a v .
	 *
	 * @parak drop true iff any dimensions of unit extent are to
	 * be removed from the result.
	 * 
	 * @return Pointer to a newly created object of type \a V ,
	 * containing the designated subset of \a v .
	 */
	template <class V>
	static V* arraySubset(const V* v, const PairList* indices,
			      bool drop);

	/** @brief Obtain canonical index vector from an IntVector.
	 *
	 * @param raw_indices Non-null pointer to an IntVector.  An
	 *          error is raised if this vector is not of one of
	 *          the two legal forms: (i) consisting entirely of
	 *          non-negative integers and/or NAs; or (ii) consisting
	 *          entirely of non-positive integers with no NAs.  In
	 *          case (i) the returned vector is obtained by
	 *          omitting any zeroes from \a raw_indices .  In case
	 *          (ii) the returned vector is the sequence (possibly
	 *          empty) from 1 to \a range_size , omitting any
	 *          values which appear (negated) within \a
	 *          raw_indices .
	 *
	 * @param range_size The size of the vector or dimension into
	 *          which indexing is being performed.
	 *
	 * @return The first element of the returned value is a
	 * pointer to the canonicalised index vector.  The second
	 * element is the minimum size implied by \a raw_indices for
	 * the vector or dimension into which indexing is being
	 * performed.  If this exceeds \a range_size it means that an
	 * attempt is being made to read from or write to elements
	 * beyond the end of the range.
	 */
	static std::pair<const IntVector*, size_t>
	canonicalize(const IntVector* raw_indices, size_t range_size);

	/** @brief Obtain canonical index vector from a LogicalVector.
	 *
	 * In the normal case, where \a raw_indices is non-empty, the
	 * action of this function can be understood by imagining that
	 * internally the function constructs an <b>effective
	 * lvector</b> comprising logical values (including NA).  This
	 * effective lvector is constructed by starting with a copy of
	 * \a raw_indices.  If the resulting lvector is smaller than
	 * \a range_size , its elements are repeated in rotation to
	 * bring it up to \a range_size .
	 *
	 * The canonical index vector is then generated by working
	 * through the elements of the effective lvector in order.  Any
	 * element that is zero is ignored. Any element that is NA
	 * results in an NA being appended to the canonical index
	 * vector.  Any element of the lvector that has any other
	 * value results in that element's position within the lvector
	 * (counting from 1) being appended to the canonical index vector.
	 *
	 * @param raw_indices Non-null pointer to a LogicalVector.  If
	 *          this is empty, the returned index vector will also
	 *          be empty.  Otherwise the behaviour is as described
	 *          above.
	 *
	 * @param range_size The size of the vector or dimension into
	 *          which indexing is being performed.
	 *
	 * @return The first element of the returned value is a
	 * pointer to the canonicalised index vector.  The second
	 * element is the minimum size implied by \a raw_indices for
	 * the vector or dimension into which indexing is being
	 * performed; this will be whichever is greater of \a
	 * range_size or the size of \a raw_indices .  If this exceeds
	 * \a range_size it means that an attempt is being made to
	 * read or write from elements beyond the end of the range.
	 */
	static std::pair<const IntVector*, size_t>
	canonicalize(const LogicalVector* raw_indices, size_t range_size);

	/** @brief Obtain canonical index vector from an StringVector.
	 *
	 * @param raw_indices Non-null pointer to a StringVector.  Any
	 *          element of this which is blank or NA will be
	 *          mapped to NA in the resulting canonical index
	 *          vector.  Any non-blank, non-NA element which
	 *          matches a name in \a range_names will be mapped to
	 *          the corresponding index.  (If there are duplicate
	 *          names in \a range_names it will be mapped to the
	 *          lowest corresponding index.)  Any non-blank,
	 *          non-NA element which does not match a name in \a
	 *          range_names will be considered to be associated
	 *          with a supplementary element of the range into
	 *          which indexing is being performed; this
	 *          supplementary element will be allocated the lowest
	 *          (positive) index number not already used.
	 *
	 * @param range_size The size of the vector or dimension into
	 *          which indexing is being performed.
	 *
	 * @param range_names Pointer, possibly null, to the vector of
	 *          names associated with the vector or dimension into
	 *          which indexing is being performed.  If present,
	 *          the size of this vector must be equal to \a
	 *          range_size .
	 *
	 * @return The first element of the returned value is a
	 * pointer to the canonicalised index vector.  The second
	 * element is normally equal to the size of \a range_names .
	 * However, if \a raw_indices contained at least one
	 * non-blank, non-null name unmatched in \a range_names , then
	 * the second element of the return value will exceed the size
	 * of \a range_names by the number of such names encountered,
	 * each of which will have been mapped to a supplementary
	 * element of the vector being indexed.  In that case, the
	 * first element of the return value will have the attribute
	 * 'use.names' set to a ListVector of the same size as \a
	 * raw_indices .  All the elements of this ListVector are
	 * NULL, except where the corresponding element of \a
	 * raw_indices was mapped into a supplementary vector element,
	 * in which case the ListVector element will be a CachedString
	 * giving the name of that supplementary vector element.
	 *
	 * @note The use.names attribute should be regarded as private
	 * to the Subscripting class, because its implementation (or
	 * indeed its use at all) may change in the future.
	 */
	static std::pair<const IntVector*, size_t>
	canonicalize(const StringVector* raw_indices, size_t range_size,
		     const StringVector* range_names);

	/** Canonicalize a subscript object for indexing an R vector.
	 *
	 * @param v Non-null pointer to the VectorBase to which
	 *          subscripting is to be applied.
	 *
	 * @param subscripts Pointer, possibly null, to an RObject.
	 *          If the type and contents of this object are legal
	 *          for subscripting, canonicalization will be
	 *          performed accordingly; otherwise an error will be
	 *          raised.
	 *
	 * @return The first element of the returned value is a
	 * pointer to the canonicalised index vector.  The second
	 * element is the minimum size implied by \a subscripts for
	 * the vector into which indexing is being performed.  If this
	 * exceeds the size of \a v it means that an attempt is being
	 * made to read or write from elements beyond the end of the
	 * vector.
	 *
	 * @note At present this function does not handle the special
	 * case (described in sec. 3.4.2 of the R Language Definition)
	 * where \a v is an array, and \a subscripts is a matrix with
	 * as many columns as \a v has dimensions, each row of the
	 * matrix being used to pick out a single element of the
	 * array.
	 */
	static std::pair<const IntVector*, size_t>
	canonicalizeVectorSubscript(const VectorBase* v,
				    const RObject* subscripts);

	/** @brief Remove dimensions of unit extent.
	 *
	 * If \a v does not point to an R matrix or array, \a v is
	 * left unchanged and the function returns false.
	 *
	 * Otherwise, the function will examine \a v to determine if
	 * it has any dimensions with just one level, in which case
	 * those dimensions are removed, and the corresponding
	 * 'dimnames' (if any) are discarded.
	 *
	 * If, after dropping dimensions, only one dimension is left,
	 * then \a v is converted to a vector, with its 'names'
	 * attribute taken from the 'dimnames' (if any) of the
	 * surviving dimension.
	 *
	 * If all the original dimensions were of unit extent, then
	 * again \a v is converted to a vector (with a single
	 * element).  This vector is given a 'names' attribute only if
	 * just one of the original dimensions had associated
	 * 'dimnames', in which case the 'names' are taken from them.
	 *
	 * @param v Non-null pointer to the vector whose dimensions
	 *          are to be examined and if necessary modified.
	 *
	 * @return true if any dimensions were dropped, otherwise
	 * false.
	 */
	static bool dropDimensions(VectorBase* v);

	template <class VL, class VR>
	static VL* vectorSubassign(VL* lhs, const IntVector* indices,
				   const VR* rhs);

	/** @brief Extract a subset of an R vector object.
	 *
	 * @tparam V A type inheriting from VectorBase.
	 *
	 * @param v Non-null pointer to a \a V object, which is the
	 *          object from which a subset (not necessarily a
	 *          proper subset) is to be extracted.
	 *
	 * @param indices Non-null pointer to a canonical index vector
	 *          designating the elements of \a v to be included as
	 *          successive elements of the output vector, which
	 *          will be the same size as \a indices .  NA_INTEGER
	 *          is a permissible index value, in which case the
	 *          corresponding element of the output vector will
	 *          have an NA value appropriate to type \a V .  If an
	 *          index is out of range with respect to \a v , in
	 *          that case also the corresponding element of the
	 *          output vector will have an NA value appropriate to
	 *          type \a V .
	 *
	 * @return Pointer to a newly created object of type \a V ,
	 * containing the designated subset of \a v .
	 */
	template <class V>
	static V* vectorSubset(const V* v, const IntVector* indices);

	/** @brief Extract a subset of an R vector object.
	 *
	 * @tparam V A type inheriting from VectorBase.
	 *
	 * @param v Non-null pointer to a \a V object, which is the
	 *          object from which a subset (not necessarily a
	 *          proper subset) is to be extracted.
	 *
	 * @param subscripts Pointer, possibly null, to an RObject  If
	 *          the type and contents of this object are legal for
	 *          subscripting, subsetting will be performed
	 *          accordingly; otherwise an error will be raised.
	 *
	 * @return Pointer to a newly created object of type \a V ,
	 * containing the designated subset of \a v .
	 */
	template <class V>
	static V* vectorSubset(const V* v, const RObject* subscripts)
	{
	    GCStackRoot<const IntVector>
		indices(canonicalizeVectorSubscript(v, subscripts).first);
	    return vectorSubset(v, indices);
	}
    private:
	// Data structure used in subsetting arrays, containing
	// information relating to a particular dimension. 
	struct DimIndexer {
	    unsigned int nindices;  // Number of index values to be
	      // extracted along this dimension.
	    const IntVector* indices;  // Pointer to array containing the index
	      // values themselves.  The index values count from 1. 
	    unsigned int indexnum;  // Position (counting from 0) of
	      // the index within 'indices' currently being processed.
	    unsigned int stride;  // Number of elements (within the
	      // linear layout of the source array) separating
	      // consecutive elements along this dimension.
	};

	typedef std::vector<DimIndexer, Allocator<DimIndexer> > DimIndexerVector;

	// Not implemented.  Declared private to prevent the
	// inadvertent creation of Subscripting objects.
	Subscripting();

	// Non-templated auxiliary function for arraySubset(), used to
	// initialise the vector of DimIndexers.  The function returns
	// the required size of the output vector.
	static size_t createDimIndexers(DimIndexerVector* dimindexers,
					const IntVector* source_dims,
					const PairList* indices);

	// If 'indices' has a 'use.names' attribute, use this to
	// update the 'names' attribute of 'v'.
	static void processUseNames(VectorBase* v, const IntVector* indices);

	// Non-templated auxiliary function for arraySubset(), used to
	// set the attributes on the result.
	static void setArrayAttributes(VectorBase* subset,
				       const VectorBase* source,
			               const DimIndexerVector& dimindexers,
				       bool drop);

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
    };  // class Subscripting

    template <class V>
    V* Subscripting::arraySubset(const V* v, const PairList* indices,
				 bool drop)
    {
	const IntVector* vdims = v->dimensions();
	size_t ndims = vdims->size();
	DimIndexerVector dimindexer(ndims);
	size_t resultsize = createDimIndexers(&dimindexer, vdims, indices);
	GCStackRoot<V> result(CXXR_NEW(V(resultsize)));
	// Copy elements across:
	{
	    // ***** FIXME *****  Currently needed because Handle's
	    // assignment operator takes a non-const RHS:
	    V* vnc = const_cast<V*>(v);
	    for (unsigned int iout = 0; iout < resultsize; ++iout) {
		bool naindex = false;
		unsigned int iin = 0;
		for (unsigned int d = 0; d < ndims; ++d) {
		    const DimIndexer& di = dimindexer[d];
		    int index = (*di.indices)[di.indexnum];
		    if (isNA(index)) {
			naindex = true;
			break;
		    }
		    if (index < 1 || index > (*vdims)[d])
			Rf_error(_("subscript out of bounds"));
		    iin += (index - 1)*di.stride;
		}
		(*result)[iout]
		    = naindex ? NA<typename V::value_type>() : (*vnc)[iin];
		// Advance the index selection:
		{
		    unsigned int d = 0;
		    bool done;
		    do {
			done = true;
			DimIndexer& di = dimindexer[d];
			if (++di.indexnum >= di.nindices) {
			    di.indexnum = 0;
			    done = (++d >= ndims);
			}
		    } while (!done);
		}
	    }
	}
	setArrayAttributes(result, v, dimindexer, drop);
	return result;
    }

    template <class VL, class VR>
    VL* Subscripting::vectorSubassign(VL* lhs, const IntVector* indices,
				      const VR* rhs)
    {
	typedef typename VL::value_type Lval;
	typedef typename VR::value_type Rval;
	size_t ni = indices->size();
	size_t rhs_size = rhs->size();
	GCStackRoot<VL> ans(lhs);
	// Make sure we don't modify rhs.  (FIXME: ideally this should
	// be a shallow copy for HandleVectors.)
	if (static_cast<VectorBase*>(lhs) == rhs)
	    ans = lhs->clone();
	for (unsigned int i = 0; i < ni; ++i) {
	    int index = (*indices)[i];
	    if (!isNA(index)) {
		const Rval& rval = (*rhs)[i % rhs_size];
		(*ans)[index - 1] = (isNA(rval) ? NA<Lval>() : Rval(rval));
	    }
	}
	processUseNames(ans, indices);
	return ans;
    }

    template <class V>
    V* Subscripting::vectorSubset(const V* v, const IntVector* indices)
    {
	size_t ni = indices->size();
	GCStackRoot<V> ans(CXXR_NEW(V(ni)));
	size_t vsize = v->size();
	// ***** FIXME *****  Currently needed because Handle's
	// assignment operator takes a non-const RHS:
	V* vnc = const_cast<V*>(v);
	for (unsigned int i = 0; i < ni; ++i) {
	    int index = (*indices)[i];
	    // Note that zero and negative indices ought not to occur.
	    if (isNA(index) || index <= 0 || index > int(vsize))
		(*ans)[i] = NA<typename V::value_type>();
	    else (*ans)[i] = (*vnc)[index - 1];
	}
	setVectorAttributes(ans, v, indices);
	return ans;
    }
}  // namespace CXXR;

#endif  // SUBSCRIPTING_HPP
