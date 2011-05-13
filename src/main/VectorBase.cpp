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
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

/** @file VectorBase.cpp
 *
 * @brief Implementation of class VectorBase and related functions.
 */

#include "CXXR/VectorBase.h"

#include "R_ext/Error.h"
#include "CXXR/IntVector.h"
#include "CXXR/ListVector.h"
#include "CXXR/StringVector.h"
#include "CXXR/Symbol.h"

using namespace std;
using namespace CXXR;

namespace CXXR {
    namespace ForceNonInline {
	int (*LENGTHptr)(SEXP x) = LENGTH;
	void (*SET_TRUELENGTHptr)(SEXP x, int v) = SET_TRUELENGTH;
	int (*TRUELENGTHptr)(SEXP x) = TRUELENGTH;
    }
}

const ListVector* VectorBase::dimensionNames() const
{
    return static_cast<const ListVector*>(getAttribute(DimNamesSymbol));
}

const StringVector* VectorBase::dimensionNames(unsigned int d) const
{
    const ListVector* lv = dimensionNames();
    if (!lv || d > lv->size())
	return 0;
    return static_cast<const StringVector*>((*lv)[d - 1].get());
}

const IntVector* VectorBase::dimensions() const
{
    return static_cast<const IntVector*>(getAttribute(DimSymbol));
}

const StringVector* VectorBase::names() const
{
    return static_cast<const StringVector*>(getAttribute(NamesSymbol));
}

void VectorBase::setDimensionNames(ListVector* names)
{
    setAttribute(DimNamesSymbol, names);
}

void VectorBase::setDimensionNames(unsigned int d, StringVector* names)
{
    unsigned int ndims = dimensions()->size();
    if (d == 0 || d > ndims)
	Rf_error(_("Attempt to associate dimnames"
		   " with a non-existent dimension"));
    ListVector* lv
	= static_cast<ListVector*>(getAttribute(DimNamesSymbol));
    if (!lv) {
	lv = CXXR_NEW(ListVector(ndims));
	setAttribute(DimNamesSymbol, lv);
    }
    (*lv)[d - 1] = names;
}

void VectorBase::setDimensions(IntVector* dims)
{
    setAttribute(DimSymbol, dims);
}

void VectorBase::setNames(StringVector* names)
{
    setAttribute(NamesSymbol, names);
}

void VectorBase::setSize(std::size_t)
{
    Rf_error(_("this object cannot be resized"));
}

// Rf_allocVector is still in memory.cpp (for the time being).

Rboolean Rf_isVector(SEXP s)
{
    switch(TYPEOF(s)) {
    case LGLSXP:
    case INTSXP:
    case REALSXP:
    case CPLXSXP:
    case STRSXP:
    case RAWSXP:

    case VECSXP:
    case EXPRSXP:
	return TRUE;
    case CXXSXP:
	return Rboolean(dynamic_cast<const VectorBase*>(s) != 0);
    default:
	return FALSE;
    }
}

void SETLENGTH(SEXP x, int v)
{
    CXXR::VectorBase* vb = dynamic_cast<CXXR::VectorBase*>(x);
    if (!vb)
	Rf_error("SETLENGTH invoked for a non-vector.");
    vb->setSize(v);
}
