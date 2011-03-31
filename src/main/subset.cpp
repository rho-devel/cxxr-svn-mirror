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
 *  Copyright (C) 1997-2007   The R Development Core Team
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
 *
 *
 *  Vector and List Subsetting
 *
 *  There are three kinds of subscripting [, [[, and $.
 *  We have three different functions to compute these.
 *
 *
 *  Note on Matrix Subscripts
 *
 *  The special [ subscripting where dim(x) == ncol(subscript matrix)
 *  is handled inside VectorSubset. The subscript matrix is turned
 *  into a subscript vector of the appropriate size and then
 *  VectorSubset continues.  This provides coherence especially
 *  regarding attributes etc. (it would be quicker to handle this case
 *  separately, but then we would have more to keep in step.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include "Defn.h"
#include "CXXR/GCStackRoot.hpp"
#include "CXXR/Subscripting.hpp"

using namespace std;
using namespace CXXR;

/* JMC convinced MM that this was not a good idea: */
#undef _S4_subsettable


/* ExtractSubset does the transfer of elements from "x" to "result" */
/* according to the integer subscripts given in "indx". */

static SEXP ExtractSubset(SEXP x, SEXP result, SEXP indx, SEXP call)
{
    int mode = TYPEOF(x);
    int n = LENGTH(indx);
    int nx = length(x);
    SEXP tmp = result;

    if (x == R_NilValue)
	return x;

    for (int i = 0; i < n; i++) {
	int ii = INTEGER(indx)[i];
	if (ii != NA_INTEGER)
	    ii--;
	switch (mode) {
	case LGLSXP:
	    if (0 <= ii && ii < nx && ii != NA_LOGICAL)
		LOGICAL(result)[i] = LOGICAL(x)[ii];
	    else
		LOGICAL(result)[i] = NA_INTEGER;
	    break;
	case INTSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		INTEGER(result)[i] = INTEGER(x)[ii];
	    else
		INTEGER(result)[i] = NA_INTEGER;
	    break;
	case REALSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		REAL(result)[i] = REAL(x)[ii];
	    else
		REAL(result)[i] = NA_REAL;
	    break;
	case CPLXSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER) {
		COMPLEX(result)[i] = COMPLEX(x)[ii];
	    }
	    else {
		COMPLEX(result)[i].r = NA_REAL;
		COMPLEX(result)[i].i = NA_REAL;
	    }
	    break;
	case STRSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		SET_STRING_ELT(result, i, STRING_ELT(x, ii));
	    else
		SET_STRING_ELT(result, i, NA_STRING);
	    break;
	case VECSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		SET_VECTOR_ELT(result, i, VECTOR_ELT(x, ii));
	    else
		SET_VECTOR_ELT(result, i, R_NilValue);
	    break;
	case EXPRSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		SET_XVECTOR_ELT(result, i, XVECTOR_ELT(x, ii));
	    else
		SET_XVECTOR_ELT(result, i, R_NilValue);
	    break;
	case LISTSXP:
	    /* cannot happen: pairlists are coerced to lists */
	case LANGSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER) {
		SEXP tmp2 = nthcdr(x, ii);
		SETCAR(tmp, CAR(tmp2));
		SET_TAG(tmp, TAG(tmp2));
	    }
	    else
		SETCAR(tmp, R_NilValue);
	    tmp = CDR(tmp);
	    break;
	case RAWSXP:
	    if (0 <= ii && ii < nx && ii != NA_INTEGER)
		RAW(result)[i] = RAW(x)[ii];
	    else
		RAW(result)[i] = Rbyte( 0);
	    break;
	default:
	    errorcall(call, R_MSG_ob_nonsub, type2char(CXXRCONSTRUCT(SEXPTYPE, mode)));
	}
    }
    return result;
}


/* This is for all cases with a single index, including 1D arrays and
   matrix indexing of arrays */
static SEXP VectorSubset(SEXP x, SEXP sarg, SEXP call)
{
    if (!x)
	return 0;
    GCStackRoot<> s(sarg);

    if (s == R_MissingArg)
	return duplicate(x);

    /* Check to see if we have special matrix subscripting. */
    /* If we do, make a real subscript vector and protect it. */
    {
	SEXP attrib = getAttrib(x, R_DimSymbol);	
	if (isMatrix(s) && isArray(x) && ncols(s) == length(attrib)) {
	    if (isString(s)) {
		s = strmat2intmat(s, GetArrayDimnames(x), call);
	    }
	    if (isInteger(s) || isReal(s)) {
		s = mat2indsub(attrib, s, call);
	    }
	}
    }

    SEXPTYPE mode = TYPEOF(x);
    switch (mode) {
    case LGLSXP:
	return Subscripting::vectorSubset(static_cast<LogicalVector*>(x), s);
    case INTSXP:
	return Subscripting::vectorSubset(static_cast<IntVector*>(x), s);
    case REALSXP:
	return Subscripting::vectorSubset(static_cast<RealVector*>(x), s);
    case CPLXSXP:
	return Subscripting::vectorSubset(static_cast<ComplexVector*>(x), s);
    case RAWSXP:
	return Subscripting::vectorSubset(static_cast<RawVector*>(x), s);
    case STRSXP:
	return Subscripting::vectorSubset(static_cast<StringVector*>(x), s);
    case VECSXP:
	return Subscripting::vectorSubset(static_cast<ListVector*>(x), s);
    case EXPRSXP:
	return Subscripting::vectorSubset(static_cast<ExpressionVector*>(x), s);
    case LANGSXP:
	break;
    default:
	errorcall(call, R_MSG_ob_nonsub, type2char(SEXPTYPE(mode)));
    }

    // If we get to here, this must be a LANGSXP.  In CXXR, this case
    // needs special handling, not least because Expression doesn't
    // inherit from VectorBase.  What follows is legacy CR code,
    // bodged as necessary.

    /* Convert to a vector of integer subscripts */
    /* in the range 1:length(x). */
    int stretch = 1;
    GCStackRoot<> indx(makeSubscript(x, s, &stretch, call));
    int n = LENGTH(indx);
    const IntVector* indices = SEXP_downcast<IntVector*>(indx.get());
    unsigned int nx = length(x);
    GCStackRoot<> result(allocVector(LANGSXP, n));
    SEXP tmp = result;
    for (unsigned int i = 0; int(i) < n; ++i) {
	int ii = (*indices)[i];
	if (ii == NA_INTEGER || ii <= 0 || ii > int(nx))
	    SETCAR(tmp, 0);
	else {
	    SEXP tmp2 = nthcdr(x, ii - 1);
	    SETCAR(tmp, CAR(tmp2));
	    SET_TAG(tmp, TAG(tmp2));
	}
	tmp = CDR(tmp);
    }
    // Fix attributes:
    {
	SEXP attrib;
	if (
	    ((attrib = getAttrib(x, R_NamesSymbol)) != R_NilValue) ||
	    ( /* here we might have an array.  Use row names if 1D */
	     isArray(x)
	     && (attrib = getAttrib(x, R_DimNamesSymbol)) != R_NilValue
	     && LENGTH(attrib) == 1
	     && (attrib = GetRowNames(attrib)) != R_NilValue
	     )
	    ) {
	    GCStackRoot<> nattrib(allocVector(TYPEOF(attrib), n));
	    nattrib = ExtractSubset(attrib, nattrib, indx, call);
	    setAttrib(result, R_NamesSymbol, nattrib);
	}
	if ((attrib = getAttrib(x, R_SrcrefSymbol)) != R_NilValue &&
	    TYPEOF(attrib) == VECSXP) {
	    GCStackRoot<> nattrib(allocVector(VECSXP, n));
	    nattrib = ExtractSubset(attrib, nattrib, indx, call);
	    setAttrib(result, R_SrcrefSymbol, nattrib);
	}
    }
    return result;
}


static SEXP ArraySubset(SEXP x, SEXP s, SEXP call, int drop)
{
    const PairList* subs = SEXP_downcast<PairList*>(s);
    switch (x->sexptype()) {
    case LGLSXP:
	return Subscripting::arraySubset(static_cast<LogicalVector*>(x),
					 subs, drop);
    case INTSXP:
	return Subscripting::arraySubset(static_cast<IntVector*>(x),
					 subs, drop);
    case REALSXP:
	return Subscripting::arraySubset(static_cast<RealVector*>(x),
					 subs, drop);
    case CPLXSXP:
	return Subscripting::arraySubset(static_cast<ComplexVector*>(x),
					 subs, drop);
    case STRSXP:
	return Subscripting::arraySubset(static_cast<StringVector*>(x),
					 subs, drop);
    case VECSXP:
	return Subscripting::arraySubset(static_cast<ListVector*>(x),
					 subs, drop);
    case RAWSXP:
	return Subscripting::arraySubset(static_cast<RawVector*>(x),
					 subs, drop);
    default:
	errorcall(call, _("array subscripting not handled for this type"));
    }
    return 0;  // -Wall
}


/* Returns and removes a named argument from argument list args.
   The search ends as soon as a matching argument is found.  If
   the argument is not found, the argument list is not modified
   and R_NilValue is returned.
 */
static SEXP ExtractArg(SEXP args, SEXP arg_sym)
{
    SEXP arg, prev_arg;
    int found = 0;

    for (arg = prev_arg = args; arg != R_NilValue; arg = CDR(arg)) {
	if(TAG(arg) == arg_sym) {
	    if (arg == prev_arg) /* found at head of args */
		args = CDR(args);
	    else
		SETCDR(prev_arg, CDR(arg));
	    found = 1;
	    break;
	}
	else  prev_arg = arg;
    }
    return found ? CAR(arg) : R_NilValue;
}

/* Extracts the drop argument, if present, from the argument list.
   The object being subsetted must be the first argument. */
static void ExtractDropArg(SEXP el, int *drop)
{
    SEXP dropArg = ExtractArg(el, R_DropSymbol);
    *drop = asLogical(dropArg);
    if (*drop == NA_LOGICAL) *drop = 1;
}


/* Extracts and, if present, removes the 'exact' argument from the
   argument list.  An integer code giving the desired exact matching
   behavior is returned:
       0  not exact
       1  exact
      -1  not exact, but warn when partial matching is used
 */
static int ExtractExactArg(SEXP args)
{
    SEXP argval = ExtractArg(args, R_ExactSymbol);
    int exact;
    if(isNull(argval)) return 1; /* Default is true as from R 2.7.0 */
    exact = asLogical(argval);
    if (exact == NA_LOGICAL) exact = -1;
    return exact;
}

/* The "[" subset operator.
 * This provides the most general form of subsetting. */

SEXP attribute_hidden do_subset(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP ans;

    /* If the first argument is an object and there is an */
    /* approriate method, we dispatch to that method, */
    /* otherwise we evaluate the arguments and fall through */
    /* to the generic code below.  Note that evaluation */
    /* retains any missing argument indicators. */

    if(DispatchOrEval(call, op, "[", args, rho, &ans, 0, 0))
/*     if(DispatchAnyOrEval(call, op, "[", args, rho, &ans, 0, 0)) */
	return(ans);

    /* Method dispatch has failed, we now */
    /* run the generic internal code. */
    return do_subset_dflt(call, op, ans, rho);
}

SEXP attribute_hidden do_subset_dflt(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    GCStackRoot<> argsrt(args);

    /* By default we drop extents of length 1 */

    /* Handle case of extracting a single element from a simple vector
       directly to improve speed for this simple case. */
    if (CDDR(args) == R_NilValue) {
	SEXP x = CAR(args);
	SEXP s = CADR(args);
	if (ATTRIB(x) == R_NilValue && ATTRIB(s) == R_NilValue) {
	    int i;
	    switch (TYPEOF(x)) {
	    case REALSXP:
		switch (TYPEOF(s)) {
		case REALSXP: i = (LENGTH(s) == 1) ? CXXRCONSTRUCT(int, REAL(s)[0]) : -1; break;
		case INTSXP: i = (LENGTH(s) == 1) ? INTEGER(s)[0] : -1; break;
		default:  i = -1;
		}
		if (i >= 1 && i <= LENGTH(x))
		    return ScalarReal( REAL(x)[i-1] );
		break;
	    case INTSXP:
		switch (TYPEOF(s)) {
		case REALSXP: i = (LENGTH(s) == 1) ? CXXRCONSTRUCT(int, REAL(s)[0]) : -1; break;
		case INTSXP: i = (LENGTH(s) == 1) ? INTEGER(s)[0] : -1; break;
		default:  i = -1;
		}
		if (i >= 1 && i <= LENGTH(x))
		    return ScalarInteger( INTEGER(x)[i-1] );
		break;
	    default: break;
	    }
	}
    }

    int drop = 1;
    ExtractDropArg(args, &drop);
    SEXP x = CAR(args);

    /* This was intended for compatibility with S, */
    /* but in fact S does not do this. */
    /* FIXME: replace the test by isNull ... ? */

    if (x == R_NilValue) {
	return x;
    }
    SEXP subs = CDR(args);
    int nsubs = length(subs);

    /* Here coerce pair-based objects into generic vectors. */
    /* All subsetting takes place on the generic vector form. */

    GCStackRoot<> ax(x);
    if (isPairList(x)) {
	SEXP dim = getAttrib(x, R_DimSymbol);
	int ndim = length(dim);
	if (ndim > 1) {
	    ax = allocArray(VECSXP, dim);
	    setAttrib(ax, R_DimNamesSymbol, getAttrib(x, R_DimNamesSymbol));
	    setAttrib(ax, R_NamesSymbol, getAttrib(x, R_DimNamesSymbol));
	}
	else {
	    ax = allocVector(VECSXP, length(x));
	    setAttrib(ax, R_NamesSymbol, getAttrib(x, R_NamesSymbol));
	}
	int i = 0;
	for (SEXP px = x; px != R_NilValue; px = CDR(px))
	    SET_VECTOR_ELT(ax, i++, CAR(px));
    }
    else if (!isVector(x))
	errorcall(call, R_MSG_ob_nonsub, type2char(TYPEOF(x)));

    /* This is the actual subsetting code. */
    /* The separation of arrays and matrices is purely an optimization. */

    GCStackRoot<> ans;
    if(nsubs < 2) {
	SEXP dim = getAttrib(x, R_DimSymbol);
	int ndim = length(dim);
	ans = VectorSubset(ax, (nsubs == 1 ? CAR(subs) : R_MissingArg), call);
	/* one-dimensional arrays went through here, and they should
	   have their dimensions dropped only if the result has
	   length one and drop == TRUE
	*/
	if(ndim == 1) {
	    int len = length(ans);

	    if(!drop || len > 1) {
		GCStackRoot<> attr(allocVector(INTSXP, 1));
		SEXP attrib = getAttrib(x, R_DimNamesSymbol);
		INTEGER(attr)[0] = length(ans);
		setAttrib(ans, R_DimSymbol, attr);
		if(attrib) {
		    /* reinstate dimnames, include names of dimnames */
		    GCStackRoot<> nattrib(duplicate(attrib));
		    SET_VECTOR_ELT(nattrib, 0,
				   getAttrib(ans, R_NamesSymbol));
		    setAttrib(ans, R_DimNamesSymbol, nattrib);
		    setAttrib(ans, R_NamesSymbol, R_NilValue);
		}
	    }
	}
    } else {
	if (nsubs != length(getAttrib(x, R_DimSymbol)))
	    errorcall(call, _("incorrect number of dimensions"));
	ans = ArraySubset(ax, subs, call, drop);
    }

    /* Note: we do not coerce back to pair-based lists. */
    /* They are "defunct" in this version of R. */

    if (TYPEOF(x) == LANGSXP) {
	ax = ans;
	{
	    ans = 0;
	    size_t len = LENGTH(ax);
	    if (len > 0) {
		GCStackRoot<PairList> tl(PairList::make(len - 1));
		ans = CXXR_NEW(Expression(0, tl));
	    }
	}
	int i = 0;
	for (SEXP px = ans; px != R_NilValue ; px = CDR(px))
	    SETCAR(px, VECTOR_ELT(ax, i++));
	if (ans)  // 2007/07/23 arr
	    {
		setAttrib(ans, R_DimSymbol, getAttrib(ax, R_DimSymbol));
		setAttrib(ans, R_DimNamesSymbol,
			  getAttrib(ax, R_DimNamesSymbol));
		setAttrib(ans, R_NamesSymbol, getAttrib(ax, R_NamesSymbol));
		SET_NAMED(ans, NAMED(ax)); /* PR#7924 */
	    }
    }
    if (ATTRIB(ans) != R_NilValue) { /* remove probably erroneous attr's */
	setAttrib(ans, R_TspSymbol, R_NilValue);
#ifdef _S4_subsettable
	if(!IS_S4_OBJECT(x))
#endif
	    setAttrib(ans, R_ClassSymbol, R_NilValue);
    }
    return ans;
}


/* The [[ subset operator.  It needs to be fast. */
/* The arguments to this call are evaluated on entry. */

SEXP attribute_hidden do_subset2(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP ans;

    /* If the first argument is an object and there is */
    /* an approriate method, we dispatch to that method, */
    /* otherwise we evaluate the arguments and fall */
    /* through to the generic code below.  Note that */
    /* evaluation retains any missing argument indicators. */

    if(DispatchOrEval(call, op, "[[", args, rho, &ans, 0, 0))
/*     if(DispatchAnyOrEval(call, op, "[[", args, rho, &ans, 0, 0)) */
	return(ans);

    /* Method dispatch has failed. */
    /* We now run the generic internal code. */

    return do_subset2_dflt(call, op, ans, rho);
}

SEXP attribute_hidden do_subset2_dflt(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP ans, dims, dimnames, indx, subs, x;
    int i, ndims, nsubs, offset = 0;
    int drop = 1, pok, exact = -1;
    int named_x;

    PROTECT(args);
    ExtractDropArg(args, &drop);
    /* Is partial matching ok?  When the exact arg is NA, a warning is
       issued if partial matching occurs.
     */
    exact = ExtractExactArg(args);
    if (exact == -1)
	pok = exact;
    else
	pok = !exact;

    x = CAR(args);

    /* This code was intended for compatibility with S, */
    /* but in fact S does not do this.	Will anyone notice? */

    if (x == R_NilValue) {
	UNPROTECT(1);
	return x;
    }

    /* Get the subscripting and dimensioning information */
    /* and check that any array subscripting is compatible. */

    subs = CDR(args);
    if(0 == (nsubs = length(subs)))
	errorcall(call, _("no index specified"));
    dims = getAttrib(x, R_DimSymbol);
    ndims = length(dims);
    if(nsubs > 1 && nsubs != ndims)
	errorcall(call, _("incorrect number of subscripts"));

    /* code to allow classes to extend environment */
    if(TYPEOF(x) == S4SXP) {
        x = R_getS4DataSlot(x, ANYSXP);
	if(x == R_NilValue)
	  errorcall(call, _("this S4 class is not subsettable"));
    }

    /* split out ENVSXP for now */
    if( TYPEOF(x) == ENVSXP ) {
      if( nsubs != 1 || !isString(CAR(subs)) || length(CAR(subs)) != 1 )
	errorcall(call, _("wrong arguments for subsetting an environment"));
      ans = findVarInFrame(x, install(translateChar(STRING_ELT(CAR(subs), 0))));
      if( TYPEOF(ans) == PROMSXP ) {
	    PROTECT(ans);
	    ans = eval(ans, R_GlobalEnv);
	    UNPROTECT(1);
      } else {
	    SET_NAMED(ans, 2);
      }

      UNPROTECT(1);
      if(ans == R_UnboundValue )
	  return(R_NilValue);
      return(ans);
    }

    /* back to the regular program */
    if (!(isVector(x) || isList(x) || isLanguage(x)))
	errorcall(call, R_MSG_ob_nonsub, type2char(TYPEOF(x)));

    named_x = NAMED(x);  /* x may change below; save this now.  See PR#13411 */

    if(nsubs == 1) { /* vector indexing */
	SEXP thesub = CAR(subs);
	int len = length(thesub);

	if (len > 1)
	    x = vectorIndex(x, thesub, 0, len-1, pok, call);
	    
	offset = get1index(thesub, getAttrib(x, R_NamesSymbol),
			   length(x), pok, len > 1 ? len-1 : -1, call);
	if (offset < 0 || offset >= length(x)) {
	    /* a bold attempt to get the same behaviour for $ and [[ */
	    if (offset < 0 && (isNewList(x) ||
			       isExpression(x) ||
			       isList(x) ||
			       isLanguage(x))) {
		UNPROTECT(1);
		return R_NilValue;
	    }
	    else errorcall(call, R_MSG_subs_o_b);
	}
    } else { /* matrix indexing */
	/* Here we use the fact that: */
	/* CAR(R_NilValue) = R_NilValue */
	/* CDR(R_NilValue) = R_NilValue */

	int ndn; /* Number of dimnames. Unlikely to be anything but
		    0 or nsubs, but just in case... */

	PROTECT(indx = allocVector(INTSXP, nsubs));
	dimnames = getAttrib(x, R_DimNamesSymbol);
	ndn = length(dimnames);
	for (i = 0; i < nsubs; i++) {
	    INTEGER(indx)[i] =
		get1index(CAR(subs), (i < ndn) ? VECTOR_ELT(dimnames, i) :
			  R_NilValue,
			  INTEGER(indx)[i], pok, -1, call);
	    subs = CDR(subs);
	    if (INTEGER(indx)[i] < 0 ||
		INTEGER(indx)[i] >= INTEGER(dims)[i])
		errorcall(call, R_MSG_subs_o_b);
	}
	offset = 0;
	for (i = (nsubs - 1); i > 0; i--)
	    offset = (offset + INTEGER(indx)[i]) * INTEGER(dims)[i - 1];
	offset += INTEGER(indx)[0];
	UNPROTECT(1);
    }

    if(isPairList(x)) {
	ans = CAR(nthcdr(x, offset));
	if (named_x > NAMED(ans))
	    SET_NAMED(ans, named_x);
    } else if(isVectorList(x)) {
	/* did unconditional duplication before 2.4.0 */
	if (x->sexptype() == EXPRSXP) {
	    ExpressionVector* ev = static_cast<ExpressionVector*>(x);
	    ans = (*ev)[offset];
	}
	else ans = VECTOR_ELT(x, offset);
	if (named_x > NAMED(ans))
	    SET_NAMED(ans, named_x);
    } else {
	ans = allocVector(TYPEOF(x), 1);
	switch (TYPEOF(x)) {
	case LGLSXP:
	case INTSXP:
	    INTEGER(ans)[0] = INTEGER(x)[offset];
	    break;
	case REALSXP:
	    REAL(ans)[0] = REAL(x)[offset];
	    break;
	case CPLXSXP:
	    COMPLEX(ans)[0] = COMPLEX(x)[offset];
	    break;
	case STRSXP:
	    SET_STRING_ELT(ans, 0, STRING_ELT(x, offset));
	    break;
	case RAWSXP:
	    RAW(ans)[0] = RAW(x)[offset];
	    break;
	default:
	    UNIMPLEMENTED_TYPE("do_subset2", x);
	}
    }
    UNPROTECT(1);
    return ans;
}


enum pmatch {
    NO_MATCH,
    EXACT_MATCH,
    PARTIAL_MATCH
};

/* A helper to partially match tags against a candidate.
   Tags are always in the native charset.
 */
/* Returns: */
static
enum pmatch
pstrmatch(SEXP target, SEXP input, int slen)
{
    const char *st = "";

    if(target == R_NilValue)
	return NO_MATCH;

    switch (TYPEOF(target)) {
    case SYMSXP:
	st = CHAR(PRINTNAME(target));
	break;
    case CHARSXP:
	st = translateChar(target);
	break;
    default:  // -Wswitch
	break;
    }
    if(strncmp(st, translateChar(input), slen) == 0)
	return (CXXRCONSTRUCT(int, strlen(st)) == slen) ?  EXACT_MATCH : PARTIAL_MATCH;
    else return NO_MATCH;
}


/* The $ subset operator.
   We need to be sure to only evaluate the first argument.
   The second will be a symbol that needs to be matched, not evaluated.
*/
SEXP attribute_hidden do_subset3(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP input, nlist, ans;

    checkArity(op, args);

    /* first translate CADR of args into a string so that we can
       pass it down to DispatchorEval and have it behave correctly */
    input = PROTECT(allocVector(STRSXP, 1));

    nlist = CADR(args);
    if(isSymbol(nlist) )
	SET_STRING_ELT(input, 0, PRINTNAME(nlist));
    else if(isString(nlist) )
	SET_STRING_ELT(input, 0, STRING_ELT(nlist, 0));
    else {
	errorcall(call,_("invalid subscript type '%s'"),
		  type2char(TYPEOF(nlist)));
    }

    /* replace the second argument with a string */

    /* Previously this was SETCADR(args, input); */
    /* which could cause problems when nlist was */
    /* ..., as in PR#8718 */
    PROTECT(args = CONS(CAR(args), CONS(input, R_NilValue)));

    /* If the first argument is an object and there is */
    /* an approriate method, we dispatch to that method, */
    /* otherwise we evaluate the arguments and fall */
    /* through to the generic code below.  Note that */
    /* evaluation retains any missing argument indicators. */

    if(DispatchOrEval(call, op, "$", args, env, &ans, 0, 0)) {
	UNPROTECT(2);
	return(ans);
    }

    UNPROTECT(2);
    return R_subset3_dflt(CAR(ans), STRING_ELT(input, 0), call);
}

/* used in eval.c */
SEXP attribute_hidden R_subset3_dflt(SEXP x, SEXP input, SEXP call)
{
    SEXP y, nlist;
    int slen;

    PROTECT(x);
    PROTECT(input);

    /* Optimisation to prevent repeated recalculation */
    slen = strlen(translateChar(input));
     /* The mechanism to allow  a class extending "environment" */
    if( IS_S4_OBJECT(x) && TYPEOF(x) == S4SXP ){
        x = R_getS4DataSlot(x, ANYSXP);
	if(x == R_NilValue)
	    errorcall(call, "$ operator not defined for this S4 class");
    }

    /* If this is not a list object we return NULL. */
    /* Or should this be allocVector(VECSXP, 0)? */

    if (isPairList(x)) {
	SEXP xmatch = R_NilValue;
	int havematch;
	UNPROTECT(2);
	havematch = 0;
	for (y = x ; y != R_NilValue ; y = CDR(y)) {
	    switch(pstrmatch(TAG(y), input, slen)) {
	    case EXACT_MATCH:
		y = CAR(y);
		if (NAMED(x) > NAMED(y)) SET_NAMED(y, NAMED(x));
		return y;
	    case PARTIAL_MATCH:
		havematch++;
		xmatch = y;
		break;
	    case NO_MATCH:
		break;
	    }
	}
	if (havematch == 1) { /* unique partial match */
	    if(R_warn_partial_match_dollar) {
		const char *st = "";
		SEXP target = TAG(y);
		switch (TYPEOF(target)) {
		case SYMSXP:
		    st = CHAR(PRINTNAME(target));
		    break;
		case CHARSXP:
		    st = translateChar(target);
		    break;
		default:
		    throw std::logic_error("Unexpected SEXPTYPE");
		}
		warningcall(call, _("partial match of '%s' to '%s'"),
			    translateChar(input), st);
	    }
	    y = CAR(xmatch);
	    if (NAMED(x) > NAMED(y)) SET_NAMED(y, NAMED(x));
	    return y;
	}
	return R_NilValue;
    }
    else if (isVectorList(x)) {
	int i, n, havematch, imatch=-1;
	nlist = getAttrib(x, R_NamesSymbol);
	UNPROTECT(2);
	n = length(nlist);
	havematch = 0;
	for (i = 0 ; i < n ; i = i + 1) {
	    switch(pstrmatch(STRING_ELT(nlist, i), input, slen)) {
	    case EXACT_MATCH:
		y = VECTOR_ELT(x, i);
		if (NAMED(x) > NAMED(y))
		    SET_NAMED(y, NAMED(x));
		return y;
	    case PARTIAL_MATCH:
		havematch++;
		if (havematch == 1) {
		    /* partial matches can cause aliasing in eval.c:evalseq
		       This is overkill, but alternative ways to prevent
		       the aliasing appear to be even worse */
		    y = VECTOR_ELT(x,i);
		    SET_NAMED(y,2);
		    SET_VECTOR_ELT(x,i,y);
		}
		imatch = i;
		break;
	    case NO_MATCH:
		break;
	    }
	}
	if(havematch == 1) { /* unique partial match */
	    if(R_warn_partial_match_dollar) {
		const char *st = "";
		SEXP target = STRING_ELT(nlist, imatch);
		switch (TYPEOF(target)) {
		case SYMSXP:
		    st = CHAR(PRINTNAME(target));
		    break;
		case CHARSXP:
		    st = translateChar(target);
		    break;
		default:
		    throw std::logic_error("Unexpected SEXPTYPE");
		}
		warningcall(call, _("partial match of '%s' to '%s'"),
			    translateChar(input), st);
	    }
	    y = VECTOR_ELT(x, imatch);
	    if (NAMED(x) > NAMED(y)) SET_NAMED(y, NAMED(x));
	    return y;
	}
	return R_NilValue;
    }
    else if( isEnvironment(x) ){
	y = findVarInFrame(x, install(translateChar(input)));
	if( TYPEOF(y) == PROMSXP ) {
	    PROTECT(y);
	    y = eval(y, R_GlobalEnv);
	    UNPROTECT(1);
	}
	UNPROTECT(2);
	if( y != R_UnboundValue ) {
	    if (NAMED(x) > NAMED(y))
		SET_NAMED(y, NAMED(x));
	    return(y);
	}
      return R_NilValue;
    }
    else if( isVectorAtomic(x) ){
	errorcall(call, "$ operator is invalid for atomic vectors");
    }
    else /* e.g. a function */
	errorcall(call, R_MSG_ob_nonsub, type2char(TYPEOF(x)));
    UNPROTECT(2);
    return R_NilValue;
}
