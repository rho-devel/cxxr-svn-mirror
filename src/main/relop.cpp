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
 *  Copyright (C) 1997--2010  The R Development Core Team
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>
#include <Rmath.h>
#include <errno.h>

#include "basedecl.h"

#include "CXXR/GCStackRoot.hpp"
#include "CXXR/VectorOps.hpp"

using namespace CXXR;
using namespace VectorOps;

static SEXP integer_relop(RELOP_TYPE code, SEXP s1, SEXP s2);
static SEXP real_relop(RELOP_TYPE code, SEXP s1, SEXP s2);
static SEXP complex_relop(RELOP_TYPE code, SEXP s1, SEXP s2, SEXP call);
static SEXP string_relop(RELOP_TYPE code, SEXP s1, SEXP s2);
static SEXP raw_relop(RELOP_TYPE code, SEXP s1, SEXP s2);

SEXP attribute_hidden do_relop(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans;

    if (DispatchGroup("Ops", call, op, args, env, &ans))
	return ans;
    checkArity(op, args);
    return do_relop_dflt(call, op, CAR(args), CADR(args));
}

SEXP attribute_hidden do_relop_dflt(SEXP call, SEXP op, SEXP xarg, SEXP yarg)
{
    GCStackRoot<> x(xarg), y(yarg);
    int nx = length(x);
    int ny = length(y);
    bool mismatch = false;

    /* pre-test to handle the most common case quickly.
       Used to skip warning too ....
     */
    if (ATTRIB(x) == R_NilValue && ATTRIB(y) == R_NilValue &&
	TYPEOF(x) == REALSXP && TYPEOF(y) == REALSXP &&
	LENGTH(x) > 0 && LENGTH(y) > 0) {
	SEXP ans = real_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
	if (nx > 0 && ny > 0)
	    mismatch = (((nx > ny) ? nx % ny : ny % nx) != 0);
	if (mismatch)
	    warningcall(call, _("longer object length is not a multiple of shorter object length"));
	return ans;
    }

    /* That symbols and calls were allowed was undocumented prior to
       R 2.5.0.  We deparse them as deparse() would, minus attributes */
    bool iS;
    if ((iS = isSymbol(x)) || TYPEOF(x) == LANGSXP) {
	SEXP tmp = allocVector(STRSXP, 1);
	SET_STRING_ELT(tmp, 0, (iS) ? PRINTNAME(x) :
		       STRING_ELT(deparse1(x, CXXRFALSE, DEFAULTDEPARSE), 0));
	x = tmp;
    }
    if ((iS = isSymbol(y)) || TYPEOF(y) == LANGSXP) {
	SEXP tmp = allocVector(STRSXP, 1);
	SET_STRING_ELT(tmp, 0, (iS) ? PRINTNAME(y) :
		       STRING_ELT(deparse1(y, CXXRFALSE, DEFAULTDEPARSE), 0));
	y = tmp;
    }

    if (!isVector(x) || !isVector(y)) {
	if (isNull(x) || isNull(y))
	    return allocVector(LGLSXP,0);
	errorcall(call,
		  _("comparison (%d) is possible only for atomic and list types"),
		  PRIMVAL(op));
    }

    if (TYPEOF(x) == EXPRSXP || TYPEOF(y) == EXPRSXP)
	errorcall(call, _("comparison is not allowed for expressions"));

    /* ELSE :  x and y are both atomic or list */

    if (LENGTH(x) <= 0 || LENGTH(y) <= 0) {
	return allocVector(LGLSXP,0);
    }

    VectorBase* xv = static_cast<VectorBase*>(x.get());
    VectorBase* yv = static_cast<VectorBase*>(y.get());
    checkOperandsConformable(xv, yv);

    if (nx > 0 && ny > 0)
	mismatch = (((nx > ny) ? nx % ny : ny % nx) != 0);
    if (mismatch)
	warningcall(call, _("longer object length is not a multiple of shorter object length"));

    GCStackRoot<> ans;
    if (isString(x) || isString(y)) {
	x = coerceVector(x, STRSXP);
	y = coerceVector(y, STRSXP);
	ans = string_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
    }
    else if (isComplex(x) || isComplex(y)) {
	x = coerceVector(x, CPLXSXP);
	y = coerceVector(y, CPLXSXP);
	ans = complex_relop(RELOP_TYPE( PRIMVAL(op)), x, y, call);
    }
    else if (isReal(x) || isReal(y)) {
	x = coerceVector(x, REALSXP);
	y = coerceVector(y, REALSXP);
	ans = real_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
    }
    else if (isInteger(x) || isInteger(y)) {
	x = coerceVector(x, INTSXP);
	y = coerceVector(y, INTSXP);
	ans = integer_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
    }
    else if (isLogical(x) || isLogical(y)) {
	x = coerceVector(x, LGLSXP);
	y = coerceVector(y, LGLSXP);
	ans = integer_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
    }
    else if (TYPEOF(x) == RAWSXP || TYPEOF(y) == RAWSXP) {
	x = coerceVector(x, RAWSXP);
	y = coerceVector(y, RAWSXP);
	ans = raw_relop(RELOP_TYPE( PRIMVAL(op)), x, y);
    } else errorcall(call, _("comparison of these types is not implemented"));


    GCStackRoot<> dims, xnames, ynames;

    bool xarray = isArray(x);
    bool yarray = isArray(y);
    if (xarray) {
	dims = getAttrib(x, R_DimSymbol);
	xnames = getAttrib(x, R_DimNamesSymbol);
    }
    if (yarray) {
	dims = getAttrib(y, R_DimSymbol);
	ynames = getAttrib(y, R_DimNamesSymbol);
    } else if (!xarray) {
	// Neither operand is an array:
	xnames = getAttrib(x, R_NamesSymbol);
	ynames = getAttrib(y, R_NamesSymbol);
    }

    if (dims != R_NilValue) {
	setAttrib(ans, R_DimSymbol, dims);
	if (xnames != R_NilValue)
	    setAttrib(ans, R_DimNamesSymbol, xnames);
	else if (ynames != R_NilValue)
	    setAttrib(ans, R_DimNamesSymbol, ynames);
    }
    else {
	if (length(x) == length(xnames))
	    setAttrib(ans, R_NamesSymbol, xnames);
	else if (length(x) == length(ynames))
	    setAttrib(ans, R_NamesSymbol, ynames);
    }

    GCStackRoot<> tsp, klass;

    bool yts = isTs(y);
    if (yts) {
	tsp = getAttrib(y, R_TspSymbol);
	klass = getAttrib(y, R_ClassSymbol);
    }

    bool xts = isTs(x);
    if (xts) {
	tsp = getAttrib(x, R_TspSymbol);
	klass = getAttrib(x, R_ClassSymbol);
    }

    if (xts || yts) {
	setAttrib(ans, R_TspSymbol, tsp);
	setAttrib(ans, R_ClassSymbol, klass);
    }

    return ans;
}

/* i1 = i % n1; i2 = i % n2;
 * this macro is quite a bit faster than having real modulo calls
 * in the loop (tested on Intel and Sparc)
 */
#define mod_iterate(n1,n2,i1,i2) for (i=i1=i2=0; i<n; \
	i1 = (++i1 == n1) ? 0 : i1,\
	i2 = (++i2 == n2) ? 0 : i2,\
	++i)

static SEXP integer_relop(RELOP_TYPE code, SEXP s1, SEXP s2)
{
    int i, i1, i2, n, n1, n2;
    int x1, x2;
    SEXP ans;

    n1 = LENGTH(s1);
    n2 = LENGTH(s2);
    n = (n1 > n2) ? n1 : n2;
    PROTECT(s1);
    PROTECT(s2);
    ans = allocVector(LGLSXP, n);

    switch (code) {
    case EQOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 == x2);
	}
	break;
    case NEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 != x2);
	}
	break;
    case LTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 < x2);
	}
	break;
    case GTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 > x2);
	}
	break;
    case LEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 <= x2);
	}
	break;
    case GEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = INTEGER(s1)[i1];
	    x2 = INTEGER(s2)[i2];
	    if (x1 == NA_INTEGER || x2 == NA_INTEGER)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 >= x2);
	}
	break;
    }
    UNPROTECT(2);
    return ans;
}

static SEXP real_relop(RELOP_TYPE code, SEXP s1, SEXP s2)
{
    int i, i1, i2, n, n1, n2;
    double x1, x2;
    SEXP ans;

    n1 = LENGTH(s1);
    n2 = LENGTH(s2);
    n = (n1 > n2) ? n1 : n2;
    PROTECT(s1);
    PROTECT(s2);
    ans = allocVector(LGLSXP, n);

    switch (code) {
    case EQOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 == x2);
	}
	break;
    case NEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 != x2);
	}
	break;
    case LTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 < x2);
	}
	break;
    case GTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 > x2);
	}
	break;
    case LEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 <= x2);
	}
	break;
    case GEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = REAL(s1)[i1];
	    x2 = REAL(s2)[i2];
	    if (ISNAN(x1) || ISNAN(x2))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1 >= x2);
	}
	break;
    }
    UNPROTECT(2);
    return ans;
}

static SEXP complex_relop(RELOP_TYPE code, SEXP s1, SEXP s2, SEXP call)
{
    int i, i1, i2, n, n1, n2;
    Rcomplex x1, x2;
    SEXP ans;

    if (code != EQOP && code != NEOP) {
	errorcall(call, _("invalid comparison with complex values"));
    }

    n1 = LENGTH(s1);
    n2 = LENGTH(s2);
    n = (n1 > n2) ? n1 : n2;
    PROTECT(s1);
    PROTECT(s2);
    ans = allocVector(LGLSXP, n);

    switch (code) {
    case EQOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = COMPLEX(s1)[i1];
	    x2 = COMPLEX(s2)[i2];
	    if (ISNAN(x1.r) || ISNAN(x1.i) ||
		ISNAN(x2.r) || ISNAN(x2.i))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1.r == x2.r && x1.i == x2.i);
	}
	break;
    case NEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = COMPLEX(s1)[i1];
	    x2 = COMPLEX(s2)[i2];
	    if (ISNAN(x1.r) || ISNAN(x1.i) ||
		ISNAN(x2.r) || ISNAN(x2.i))
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = (x1.r != x2.r || x1.i != x2.i);
	}
	break;
    default:
	/* never happens (-Wall) */
	break;
    }
    UNPROTECT(2);
    return ans;
}


/* POSIX allows EINVAL when one of the strings contains characters
   outside the collation domain. */
static SEXP string_relop(RELOP_TYPE code, SEXP s1, SEXP s2)
{
    int i, n, n1, n2, res;
    SEXP ans, c1, c2;

    n1 = LENGTH(s1);
    n2 = LENGTH(s2);
    n = (n1 > n2) ? n1 : n2;
    PROTECT(s1);
    PROTECT(s2);
    PROTECT(ans = allocVector(LGLSXP, n));

    switch (code) {
    case EQOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = Seql(c1, c2) ? 1 : 0;
	}
	break;
    case NEOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else
		LOGICAL(ans)[i] = Seql(c1, c2) ? 0 : 1;
	}
	break;
    case LTOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else if (c1 == c2)
		LOGICAL(ans)[i] = 0;
	    else {
		errno = 0;
		res = Scollate(c1, c2);
		if(errno)
		    LOGICAL(ans)[i] = NA_LOGICAL;
		else
		    LOGICAL(ans)[i] = (res < 0) ? 1 : 0;
	    }
	}
	break;
    case GTOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else if (c1 == c2)
		LOGICAL(ans)[i] = 0;
	    else {
		errno = 0;
		res = Scollate(c1, c2);
		if(errno)
		    LOGICAL(ans)[i] = NA_LOGICAL;
		else
		    LOGICAL(ans)[i] = (res > 0) ? 1 : 0;
	    }
	}
	break;
    case LEOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else if (c1 == c2)
		LOGICAL(ans)[i] = 1;
	    else {
		errno = 0;
		res = Scollate(STRING_ELT(s1, i % n1), STRING_ELT(s2, i % n2));
		if(errno)
		    LOGICAL(ans)[i] = NA_LOGICAL;
		else
		    LOGICAL(ans)[i] = (res <= 0) ? 1 : 0;
	    }
	}
	break;
    case GEOP:
	for (i = 0; i < n; i++) {
	    c1 = STRING_ELT(s1, i % n1);
	    c2 = STRING_ELT(s2, i % n2);
	    if (c1 == NA_STRING || c2 == NA_STRING)
		LOGICAL(ans)[i] = NA_LOGICAL;
	    else if (c1 == c2)
		LOGICAL(ans)[i] = 1;
	    else {
		errno = 0;
		res = Scollate(STRING_ELT(s1, i % n1), STRING_ELT(s2, i % n2));
		if(errno)
		    LOGICAL(ans)[i] = NA_LOGICAL;
		else
		    LOGICAL(ans)[i] = (res >= 0) ? 1 : 0;
	    }
	}
	break;
    }
    UNPROTECT(3);
    return ans;
}

static SEXP raw_relop(RELOP_TYPE code, SEXP s1, SEXP s2)
{
    int i, i1, i2, n, n1, n2;
    Rbyte x1, x2;
    SEXP ans;

    n1 = LENGTH(s1);
    n2 = LENGTH(s2);
    n = (n1 > n2) ? n1 : n2;
    PROTECT(s1);
    PROTECT(s2);
    ans = allocVector(LGLSXP, n);

    switch (code) {
    case EQOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 == x2);
	}
	break;
    case NEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 != x2);
	}
	break;
    case LTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 < x2);
	}
	break;
    case GTOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 > x2);
	}
	break;
    case LEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 <= x2);
	}
	break;
    case GEOP:
	mod_iterate(n1, n2, i1, i2) {
	    x1 = RAW(s1)[i1];
	    x2 = RAW(s2)[i2];
	    LOGICAL(ans)[i] = (x1 >= x2);
	}
	break;
    }
    UNPROTECT(2);
    return ans;
}


SEXP bitwiseAnd(SEXP a, SEXP b)
{
    int  m = LENGTH(a), n = LENGTH(b), mn = (m && n) ? fmax2(m, n) : 0;
    SEXP ans = allocVector(INTSXP, mn);
    for(int i = 0; i < mn; i++)
	INTEGER(ans)[i] = INTEGER(a)[i%m] & INTEGER(b)[i%n];
    return ans;
}

SEXP bitwiseOr(SEXP a, SEXP b)
{
    int  m = LENGTH(a), n = LENGTH(b), mn = (m && n) ? fmax2(m, n) : 0;
    SEXP ans = allocVector(INTSXP, mn);
    for(int i = 0; i < mn; i++)
	INTEGER(ans)[i] = INTEGER(a)[i%m] | INTEGER(b)[i%n];
    return ans;
}

SEXP bitwiseXor(SEXP a, SEXP b)
{
    int  m = LENGTH(a), n = LENGTH(b), mn = (m && n) ? fmax2(m, n) : 0;
    SEXP ans = allocVector(INTSXP, mn);
    for(int i = 0; i < mn; i++)
	INTEGER(ans)[i] = INTEGER(a)[i%m] ^ INTEGER(b)[i%n];
    return ans;
}
