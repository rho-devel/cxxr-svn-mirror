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
 *  R : A Computer Langage for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 2001-4  The R Development Core Team
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

/* <UTF8> char here is either ASCII or handled as a whole */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Defn.h"
#include "CXXR/GCStackRoot.hpp"

using namespace CXXR;

R_len_t Rf_length(SEXP s)
{
    if (Rf_isVector(s))
	return LENGTH(s);
    switch (TYPEOF(s)) {
    case NILSXP:
	return 0;
    case LISTSXP:
    case LANGSXP:
    case DOTSXP:
	{
	    int i = 0;
	    while (s != NULL && s != R_NilValue) {
		i++;
		s = CDR(s);
	    }
	    return i;
	}
    case ENVSXP:
	return Rf_envlength(s);
    default:
	return 1;
    }
}

/* This is called by function() {}, where an invalid
   body should be impossible. When called from
   other places (eg do_asfunction) they
   should do this checking in advance */

/*  mkCLOSXP - return a closure with formals f,  */
/*             body b, and environment rho       */

SEXP attribute_hidden Rf_mkCLOSXP(SEXP formals, SEXP body, SEXP rho)
{
    GCStackRoot<PairList> formrt(SEXP_downcast<PairList*>(formals));
    GCStackRoot<> bodyrt(body);
    GCStackRoot<Environment> envrt(rho ? SEXP_downcast<Environment*>(rho)
				   : Environment::global());
    if (!isList(body) && !isLanguage(body) && !isSymbol(body)
	&& !isExpression(body) && !isVector(body)
#ifdef BYTECODE
	&& !isByteCode(body)
#endif
	)
	Rf_error(_("invalid body argument for \"function\"\n"
		   "Should NEVER happen; please bug.report() [mkCLOSXP]"));
    return CXXR_NEW(Closure(formrt, bodyrt, envrt));
}
