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

/** @file Subscripting.cpp
 *
 * Implementation of class CXXR::Subscripting and associated functions.
 */

#include <set>
#include "CXXR/Subscripting.hpp"

using namespace std;
using namespace CXXR;

const StringVector* dimensionNames(const VectorBase* v, unsigned int d)
{
    const ListVector* lv = dimensionNames(v);
    if (!lv || d > lv->size())
	return 0;
    return static_cast<const StringVector*>((*lv)[d - 1].get());
}

void setDimensionNames(VectorBase* v, unsigned int d, VectorBase* names)
{
    unsigned int ndims = dimensions(v)->size();
    if (d == 0 || d > ndims)
	Rf_error(_("Attempt to associate dimnames"
		   " with a non-existent dimension"));
    ListVector* lv
	= static_cast<ListVector*>(v->getAttribute(DimNamesSymbol));
    if (!lv) {
	lv = CXXR_NEW(ListVector(ndims));
	v->setAttribute(DimNamesSymbol, lv);
    }
    (*lv)[d - 1] = names;
}

pair<const IntVector*, size_t>
Subscripting::canonicalize(const IntVector* raw_indices, size_t vector_size,
			   bool keep_overshoot)
{
    const size_t rawsize = raw_indices->size();
    bool anyNA = false;
    bool anyneg = false;
    unsigned int zeroes = 0;
    unsigned int max_index = 0;
    for (unsigned int i = 0; i < rawsize; ++i) {
	int index = (*raw_indices)[i];
	if (isNA(index))
	    anyNA = true;
	else if (index < 0)
	    anyneg = true;
	else if (index == 0)
	    ++zeroes;
	else if (index > int(max_index))
	    max_index = index;
    }
    if (!anyneg) {
	// Check if raw_indices is already in the required form:
	if (zeroes == 0 && (keep_overshoot || max_index <= vector_size))
	    return make_pair(raw_indices, max_index);
	// Otherwise suppress zeroes and apply keep_overshoot:
	GCStackRoot<IntVector> ans(CXXR_NEW(IntVector(rawsize - zeroes)));
	unsigned int iout = 0;
	if (keep_overshoot) {
	    for (unsigned int iin = 0; iin < rawsize; ++iin) {
		int index = (*raw_indices)[iin];
		if (index != 0)
		    (*ans)[iout++] = index;
	    }
	} else {
	    for (unsigned int iin = 0; iin < rawsize; ++iin) {
		int index = (*raw_indices)[iin];
		if (index != 0) {
		    if (!isNA(index) && index > int(vector_size))
			index = NA<int>();
		    (*ans)[iout++] = index;
		}
	    }
	}
	return pair<const IntVector*, size_t>(ans, max_index);
    } else {  // Negative subscripts
	if (anyNA || max_index > 0)
	    Rf_error(_("only 0's may be mixed with negative subscripts"));
	set<unsigned int> excluded;
	// Build set of excluded values:
	for (unsigned int i = 0; i < rawsize; ++i) {
	    int index = -(*raw_indices)[i];
	    if (index != 0 && index <= int(vector_size))
		excluded.insert(index);
	}
	GCStackRoot<IntVector>
	    ans(CXXR_NEW(IntVector(vector_size - excluded.size())));
	// Fill answer and recompute max_index:
	{
	    unsigned int iout = 0;
	    max_index = 0;
	    for (int index = 1; index <= int(vector_size); ++index) {
		if (excluded.count(index) == 0) {
		    (*ans)[iout++] = index;
		    max_index = index;
		}
	    }
	}
	return pair<const IntVector*, size_t>(ans, max_index);
    }
}

size_t Subscripting::createDimIndexers(DimIndexerVector* dimindexers,
				       const IntVector* source_dims,
				       const PairList* indices)
{
    size_t ndims = source_dims->size();
    size_t resultsize = 1;
    const PairList* pl = indices;
    for (unsigned int d = 0; d < ndims; ++d) {
	DimIndexer& di = (*dimindexers)[d];
	const IntVector* iv = static_cast<IntVector*>(pl->car());
	di.nindices = iv->size();
	resultsize *= di.nindices;
	di.indices = iv;
	di.indexnum = 0;
	pl = pl->tail();
    }
    (*dimindexers)[0].stride = 1;
    for (unsigned int d = 1; d < ndims; ++d)
	(*dimindexers)[d].stride
	    = (*dimindexers)[d - 1].stride * (*source_dims)[d - 1];
    return resultsize;
}

bool Subscripting::dropDimensions(VectorBase* v)
{
    GCStackRoot<const IntVector> dims(dimensions(v));
    if (!dims)
	return false;
    size_t ndims = dims->size();
    // Count the number of dimensions with extent != 1 :
    size_t ngooddims = 0;
    for (unsigned int d = 0; d < ndims; ++d)
	if ((*dims)[d] != 1)
	    ++ngooddims;
    if (ngooddims == ndims)
	return false;
    ListVector* dimnames = const_cast<ListVector*>(dimensionNames(v));
    if (ngooddims > 1) {
	// The result will still be an array/matrix.
	bool havenames = false;
	// Set up new dimensions attribute:
	{
	    GCStackRoot<IntVector> newdims(CXXR_NEW(IntVector(ngooddims)));
	    unsigned int dout = 0;
	    for (unsigned int din = 0; din < ndims; ++din) {
		size_t dsize = (*dims)[din];
		if (dsize != 1) {
		    (*newdims)[dout++] = dsize;
		    if (dimnames && (*dimnames)[din])
			havenames = true;
		}
	    }
	    setDimensions(v, newdims);
	}
	if (havenames) {
	    // Set up new dimnames attribute:
	    StringVector* dimnamesnames
		= const_cast<StringVector*>(names(dimnames));
	    GCStackRoot<ListVector> newdimnames(CXXR_NEW(ListVector(ngooddims)));
	    GCStackRoot<StringVector>
		newdimnamesnames(CXXR_NEW(StringVector(ngooddims)));
	    unsigned int dout = 0;
	    for (unsigned int din = 0; din < ndims; ++din)
		if ((*dims)[din] != 1) {
		    (*newdimnames)[dout] = (*dimnames)[din];
		    if (dimnamesnames)
			(*newdimnamesnames)[dout] = (*dimnamesnames)[din];
		    ++dout;
		}
	    if (dimnamesnames)
		setNames(newdimnames, newdimnamesnames);
	    setDimensionNames(v, newdimnames);
	}
    } else if (ngooddims == 1) {
	// Reduce to a vector.
	setDimensions(v, 0);
	setDimensionNames(v, 0);
	if (dimnames) {
	    unsigned int d = 0;
	    while ((*dims)[d] == 1)
		++d;
	    setNames(v, static_cast<StringVector*>((*dimnames)[d].get()));
	}
    } else /* ngooddims == 0 */ {
	setDimensions(v, 0);
	setDimensionNames(v, 0);
	// In this special case it is ambiguous which dimnames to use
	// for the sole remaining element, so we set up a name only if
	// just one dimension has names.
	if (dimnames) {
	    StringVector* newnames;
	    unsigned int count = 0;
	    for (unsigned int d = 0; d < ndims; ++d) {
		RObject* dnd = (*dimnames)[d];
		if (dnd) {
		    newnames = static_cast<StringVector*>(dnd);
		    ++count;
		}
	    }
	    if (count == 1)
		setNames(v, newnames);
	}
    }
    return true;
}

void Subscripting::setArrayAttributes(VectorBase* subset,
				      const VectorBase* source,
				      const DimIndexerVector& dimindexers,
				      bool drop)
{
    size_t ndims = dimensions(source)->size();
    // Dimensions:
    {
	GCStackRoot<IntVector> newdims(CXXR_NEW(IntVector(ndims)));
	for (unsigned int d = 0; d < ndims; ++d)
	    (*newdims)[d] = dimindexers[d].nindices;
	setDimensions(subset, newdims);
    }
    // Dimnames:
    {
	const ListVector* dimnames = dimensionNames(source);
	if (dimnames) {
	    GCStackRoot<ListVector> newdimnames(CXXR_NEW(ListVector(ndims)));
	    for (unsigned int d = 0; d < ndims; ++d) {
		const DimIndexer& di = dimindexers[d];
		// 0-length dims have NULL dimnames:
		if (di.nindices > 0) {
		    const StringVector* sv
			= static_cast<const StringVector*>((*dimnames)[d].get());
		    if (sv)
			(*newdimnames)[d] = vectorSubset(sv, di.indices);
		}
	    }
	    const StringVector* dimnamesnames = names(dimnames);
	    if (dimnamesnames)
		setNames(newdimnames, dimnamesnames->clone());
	    setDimensionNames(subset, newdimnames);
	}
    }
    if (drop)
	dropDimensions(subset);
}

void Subscripting::setVectorAttributes(VectorBase* subset,
				       const VectorBase* source,
				       const IntVector* indices)
{
    // Names:
    {
	const StringVector* sourcenames = names(source);
	if (!sourcenames) {
	    // Use row names if this is a one-dimensional array:
	    const ListVector* dimnames = dimensionNames(source);
	    if (dimnames && dimnames->size() == 1)
		sourcenames = static_cast<const StringVector*>((*dimnames)[0].get());
	}
	if (sourcenames)
	    setNames(subset, vectorSubset(sourcenames, indices));
    }
    // R_SrcrefSymbol:
    {
	RObject* attrib = source->getAttribute(SrcrefSymbol);
	if (attrib && attrib->sexptype() == VECSXP) {
	    const ListVector* srcrefs = static_cast<const ListVector*>(attrib);
	    subset->setAttribute(SrcrefSymbol, vectorSubset(srcrefs, indices));
	}
    }
}
