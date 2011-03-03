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

#include "CXXR/Subscripting.hpp"

using namespace std;
using namespace CXXR;

const StringVector* dimensionNames(const VectorBase* v, unsigned int d)
{
    const ListVector* lv = dimensionNames(v);
    if (!lv || d > lv->size())
	return 0;
    return static_cast<const StringVector*>((*lv)[d - 1]);
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

void Subscripting::setArrayAttributes(VectorBase* subset,
				      const VectorBase* source,
				      const DimIndexerVector& dimindexers)
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
			= static_cast<const StringVector*>((*dimnames)[d]);
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
		sourcenames = static_cast<const StringVector*>((*dimnames)[0]);
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
