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
 *  Copyright (C) 1999--2010  The R Development Core Team.
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

#include "Defn.h"

#include "CXXR/GCStackRoot.hpp"
#include "CXXR/RawVector.h"
#include "CXXR/VectorOps.hpp"

using namespace CXXR;
using namespace VectorOps;

static SEXP lunary(SEXP, SEXP, SEXP);
static SEXP lbinary(SEXP, SEXP, SEXP);

namespace {
    struct AndOp {
	int operator()(int l, int r) const
	{
	    if (l == 0 || r == 0)
		return 0;
	    if (isNA(l) || isNA(r))
		return ElementTraits<int>::NA();
	    return 1;
	}
    };

    struct OrOp {
	int operator()(int l, int r) const
	{
	    if ((!isNA(l) && l != 0)
		|| (!isNA(r) && r != 0))
		return 1;
	    if (isNA(l) || isNA(r))
		return ElementTraits<int>::NA();
	    return 0;
	}
    };

    LogicalVector* binaryLogic(int opcode, const LogicalVector* l,
			       const LogicalVector* r)
    {
	switch (opcode) {
	case 1:
	    {
		BinaryFunction<GeneralBinaryAttributeCopier, AndOp,
		               NullBinaryFunctorWrapper> bf;
		return bf.apply<LogicalVector>(l, r);
	    }
	case 2:
	    {
		BinaryFunction<GeneralBinaryAttributeCopier, OrOp,
		               NullBinaryFunctorWrapper> bf;
		return bf.apply<LogicalVector>(l, r);
	    }
	case 3:
	    Rf_error(_("Unary operator `!' called with two arguments"));
	}
	return 0;  // -Wall
    }

    struct BitwiseAndOp {
	int operator()(int l, int r) const
	{
	    return l & r;
	}
    };

    struct BitwiseOrOp {
	int operator()(int l, int r) const
	{
	    return l | r;
	    return 0;
	}
    };

    RawVector* bitwiseLogic(int opcode, const RawVector* l, const RawVector* r)
    {
	switch (opcode) {
	case 1:
	    {
		BinaryFunction<GeneralBinaryAttributeCopier, BitwiseAndOp,
		               NullBinaryFunctorWrapper> bf;
		return bf.apply<RawVector>(l, r);
	    }
	case 2:
	    {
		BinaryFunction<GeneralBinaryAttributeCopier, BitwiseOrOp,
		               NullBinaryFunctorWrapper> bf;
		return bf.apply<RawVector>(l, r);
	    }
	case 3:
	    Rf_error(_("Unary operator `!' called with two arguments"));
	}
	return 0;  // -Wall
    }
}

/* & | ! */
SEXP attribute_hidden do_logic(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans;

    if (DispatchGroup("Ops",call, op, args, env, &ans))
	return ans;
    switch (length(args)) {
    case 1:
	return lunary(call, op, CAR(args));
    case 2:
	return lbinary(call, op, args);
    default:
	error(_("binary operations require two arguments"));
	return R_NilValue;	/* for -Wall */
    }
}

#define isRaw(x) (TYPEOF(x) == RAWSXP)
static SEXP lbinary(SEXP call, SEXP op, SEXP args)
{
/* logical binary : "&" or "|" */
    SEXP x = CAR(args);
    SEXP y = CADR(args);
    if (isRaw(x) && isRaw(y)) {
    }
    else if (!isNumber(x) || !isNumber(y))
	errorcall(call,
		  _("operations are possible only for"
		    " numeric, logical or complex types"));

    if (isRaw(x) && isRaw(y)) {
	RawVector* vl = static_cast<RawVector*>(x);
	RawVector* vr = static_cast<RawVector*>(y);
	return bitwiseLogic(PRIMVAL(op), vl, vr);
    } else {
	GCStackRoot<LogicalVector>
	    vl(static_cast<LogicalVector*>(coerceVector(x, LGLSXP)));
	GCStackRoot<LogicalVector>
	    vr(static_cast<LogicalVector*>(coerceVector(y, LGLSXP)));
	return binaryLogic(PRIMVAL(op), vl, vr);
    }
}

// Note that this specifically implements logical (or for RAWSXP,
// bitwise) negation, not a general unary function: the op arg is
// ignored.

static SEXP lunary(SEXP call, SEXP op, SEXP arg)
{
    SEXP x, dim, dimnames, names;
    int i, len;

    len = LENGTH(arg);
    if (!isLogical(arg) && !isNumber(arg) && !isRaw(arg)) {
	/* For back-compatibility */
	if (!len) return allocVector(LGLSXP, 0);
	errorcall(call, _("invalid argument type"));
    }
    PROTECT(names = getAttrib(arg, R_NamesSymbol));
    PROTECT(dim = getAttrib(arg, R_DimSymbol));
    PROTECT(dimnames = getAttrib(arg, R_DimNamesSymbol));
    PROTECT(x = allocVector(isRaw(arg) ? RAWSXP : LGLSXP, len));
    switch(TYPEOF(arg)) {
    case LGLSXP:
	for (i = 0; i < len; i++)
	    LOGICAL(x)[i] = (LOGICAL(arg)[i] == NA_LOGICAL) ?
		NA_LOGICAL : LOGICAL(arg)[i] == 0;
	break;
    case INTSXP:
	for (i = 0; i < len; i++)
	    LOGICAL(x)[i] = (INTEGER(arg)[i] == NA_INTEGER) ?
		NA_LOGICAL : INTEGER(arg)[i] == 0;
	break;
    case REALSXP:
	for (i = 0; i < len; i++)
	    LOGICAL(x)[i] = ISNAN(REAL(arg)[i]) ?
		NA_LOGICAL : REAL(arg)[i] == 0;
	break;
    case CPLXSXP:
	for (i = 0; i < len; i++)
	    LOGICAL(x)[i] = (ISNAN(COMPLEX(arg)[i].r) || ISNAN(COMPLEX(arg)[i].i))
		? NA_LOGICAL : (COMPLEX(arg)[i].r == 0. && COMPLEX(arg)[i].i == 0.);
	break;
    case RAWSXP:
	for (i = 0; i < len; i++)
	    RAW(x)[i] = 0xFF ^ RAW(arg)[i];
	break;
    default:
	UNIMPLEMENTED_TYPE("lunary", arg);
    }
    if(names != R_NilValue) setAttrib(x, R_NamesSymbol, names);
    if(dim != R_NilValue) setAttrib(x, R_DimSymbol, dim);
    if(dimnames != R_NilValue) setAttrib(x, R_DimNamesSymbol, dimnames);
    UNPROTECT(4);
    return x;
}

/* && || */
SEXP attribute_hidden do_logic2(SEXP call, SEXP op, SEXP args, SEXP env)
{
/*  &&	and  ||	 */
    SEXP s1, s2;
    int x1, x2;
    SEXP ans;

    if (length(args) != 2)
	error(_("'%s' operator requires 2 arguments"),
	      PRIMVAL(op) == 1 ? "&&" : "||");

    s1 = CAR(args);
    s2 = CADR(args);
    PROTECT(ans = allocVector(LGLSXP, 1));
    s1 = eval(s1, env);
    if (!isNumber(s1))
	errorcall(call, _("invalid 'x' type in 'x %s y'"),
		  PRIMVAL(op) == 1 ? "&&" : "||");
    x1 = asLogical(s1);

#define get_2nd							\
	s2 = eval(s2, env);					\
	if (!isNumber(s2))					\
	    errorcall(call, _("invalid 'y' type in 'x %s y'"),	\
		      PRIMVAL(op) == 1 ? "&&" : "||");		\
	x2 = asLogical(s2);

    switch (PRIMVAL(op)) {
    case 1: /* && */
	if (x1 == FALSE)
	    LOGICAL(ans)[0] = FALSE;
	else {
	    get_2nd;
	    if (x1 == NA_LOGICAL)
		LOGICAL(ans)[0] = (x2 == NA_LOGICAL || x2) ? NA_LOGICAL : x2;
	    else /* x1 == TRUE */
		LOGICAL(ans)[0] = x2;
	}
	break;
    case 2: /* || */
	if (x1 == TRUE)
	    LOGICAL(ans)[0] = TRUE;
	else {
	    get_2nd;
	    if (x1 == NA_LOGICAL)
		LOGICAL(ans)[0] = (x2 == NA_LOGICAL || !x2) ? NA_LOGICAL : x2;
	    else /* x1 == FALSE */
		LOGICAL(ans)[0] = x2;
	}
    }
    UNPROTECT(1);
    return ans;
}


#define _OP_ALL 1
#define _OP_ANY 2

static int checkValues(int op, int na_rm, int * x, int n)
{
    int i;
    int has_na = 0;
    for (i = 0; i < n; i++) {
        if (!na_rm && x[i] == NA_LOGICAL) has_na = 1;
        else {
            if (x[i] == TRUE && op == _OP_ANY) return TRUE;
            if (x[i] == FALSE && op == _OP_ALL) return FALSE;
	}
    }
    switch (op) {
    case _OP_ANY:
        return has_na ? NA_LOGICAL : FALSE;
    case _OP_ALL:
        return has_na ? NA_LOGICAL : TRUE;
    default:
        error("bad op value for do_logic3");
    }
    return NA_LOGICAL; /* -Wall */
}

extern SEXP fixup_NaRm(SEXP args); /* summary.c */

/* all, any */
SEXP attribute_hidden do_logic3(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans, s, t, call2;
    int narm, has_na = 0;
    /* initialize for behavior on empty vector
       all(logical(0)) -> TRUE
       any(logical(0)) -> FALSE
     */
    Rboolean val = PRIMVAL(op) == _OP_ALL ? TRUE : FALSE;

    PROTECT(args = fixup_NaRm(args));
    PROTECT(call2 = duplicate(call));
    SETCDR(call2, args);

    if (DispatchGroup("Summary", call2, op, args, env, &ans)) {
	UNPROTECT(2);
	return(ans);
    }

    ans = matchArgExact(R_NaRmSymbol, &args);
    narm = asLogical(ans);

    for (s = args; s != R_NilValue; s = CDR(s)) {
	t = CAR(s);
	/* Avoid memeory waste from coercing empty inputs, and also
	   avoid warnings with empty lists coming from sapply */
	if(length(t) == 0) continue;
	/* coerceVector protects its argument so this actually works
	   just fine */
	if (TYPEOF(t)  != LGLSXP) {
	    /* Coercion of integers seems reasonably safe, but for
	       other types it is more often than not an error.
	       One exception is perhaps the result of lapply, but
	       then sapply was often what was intended. */
	    if(TYPEOF(t) != INTSXP)
		warningcall(call,
			    _("coercing argument of type '%s' to logical"),
			    type2char(TYPEOF(t)));
	    t = coerceVector(t, LGLSXP);
	}
	val = CXXRCONSTRUCT(Rboolean, checkValues(PRIMVAL(op), narm, LOGICAL(t), LENGTH(t)));
        if (val != NA_LOGICAL) {
            if ((PRIMVAL(op) == _OP_ANY && val)
                || (PRIMVAL(op) == _OP_ALL && !val)) {
                has_na = 0;
                break;
            }
        } else has_na = 1;
    }
    UNPROTECT(2);
    return has_na ? ScalarLogical(NA_LOGICAL) : ScalarLogical(val);
}
#undef _OP_ALL
#undef _OP_ANY

namespace CXXR {
    namespace VectorOps {
	void checkOperandsConformable(const VectorBase* vl, const VectorBase* vr)
	{
	    // Temporary kludge:
	    VectorBase* vlnc = const_cast<VectorBase*>(vl);
	    VectorBase* vrnc = const_cast<VectorBase*>(vr);
	    if (Rf_isArray(vlnc) && Rf_isArray(vrnc)
		&& !Rf_conformable(vlnc, vrnc))
		Rf_error(_("non-conformable arrays"));
	    if (isTs(vlnc)) {
		if (isTs(vrnc) && !Rf_tsConform(vlnc, vrnc))
		    Rf_error(_("non-conformable time-series"));
		if (vr->size() > vl->size())
		    Rf_error(_("time-series/vector length mismatch"));
	    } else if (isTs(vrnc) && vl->size() > vr->size())
		Rf_error(_("time-series/vector length mismatch"));
	}
    } // namespace VectorOps
} // namespace CXXR  
