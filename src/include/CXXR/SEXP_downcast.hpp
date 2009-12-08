/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-9 Andrew R. Runnalls, subject to such other
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

/** @file SEXP_downcast.hpp
 * @brief The templated function CXXR::SEXP_downcast().
 */
#ifndef SEXP_DOWNCAST_HPP
#define SEXP_DOWNCAST_HPP 1

#include "CXXR/RObject.h"

namespace CXXR {
#ifdef UNCHECKED_SEXP_DOWNCAST
    template <typename Ptr>
    inline Ptr SEXP_downcast(SEXP s)
    {
	return static_cast<Ptr>(s);
    }
#else
    void SEXP_downcast_error(const char* given, const char* wanted);

    /** Down cast from RObject* to a pointer to a class derived from
     *  RObject.
     * @param Ptr Cast the pointer to type Ptr, where Ptr is a pointer
     *          or const pointer to RObject or a class derived from
     *          RObject.
     * @param s The pointer to be cast.
     * @return The cast pointer.
     */
    template <typename Ptr>
    Ptr SEXP_downcast(SEXP s)
    {
	if (!s) return 0;
	Ptr ans = dynamic_cast< Ptr >(s);
	if (!ans) 
		SEXP_downcast_error(s->typeName(), ans->staticTypeName());
	return ans;
    }
#endif
}  // namespace CXXR

#endif  // SEXP_DOWNCAST_HPP
