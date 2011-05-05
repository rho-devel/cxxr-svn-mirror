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

/** @file ListVector.h
 * @brief Class CXXR::ListVector and associated C interface.
 *
 * (ListVector implements VECSXP.)
 */

#ifndef LISTVECTOR_H
#define LISTVECTOR_H

#include "CXXR/VectorBase.h"

#ifdef __cplusplus

#include "CXXR/FixedVector.hpp"
#include "CXXR/SEXP_downcast.hpp"

namespace CXXR {
    // Template specializations:
    template <>
    inline const char* FixedVector<RHandle<>, VECSXP>::staticTypeName()
    {
	return "list";
    }

    /** @brief General vector of RHandle<RObject>.
     */
    typedef FixedVector<RHandle<>, VECSXP> ListVector;
}  // namespace CXXR

extern "C" {
#endif /* __cplusplus */

/** @brief Set element of CXXR::ListVector.
 *
 * @param x Pointer to a CXXR::ListVector.
 *
 * @param i Index of the required element.  There is no bounds checking.
 *
 * @param v Pointer, possibly null, to CXXR::RObject representing the
 *          new value.
 *
 * @return The new value \a v.
 */
SEXP SET_VECTOR_ELT(SEXP x, int i, SEXP v);

/** @brief Examine element of CXXR::ListVector.
 *
 * @param x Non-null pointer to a CXXR::ListVector .
 *
 * @param i Index of the required element.  There is no bounds checking.
 *
 * @return The value of the \a i 'th element.
 */
#ifndef __cplusplus
SEXP VECTOR_ELT(SEXP x, int i);
#else
inline SEXP VECTOR_ELT(SEXP x, int i)
{
    using namespace CXXR;
    ListVector* lv = SEXP_downcast<ListVector*>(x, false);
    return (*lv)[i];
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* LISTVECTOR_H */
