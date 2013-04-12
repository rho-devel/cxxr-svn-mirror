/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-13 Andrew R. Runnalls, subject to such other
 *CXXR copyrights and copyright restrictions as may be stated below.
 *CXXR 
 *CXXR CXXR is not part of the R project, and bugs and other issues should
 *CXXR not be reported via r-bugs or other R project channels; instead refer
 *CXXR to the CXXR website.
 *CXXR */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1999-2012  The R Core Team.
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
 *
 *  Environments:
 *
 *  All the action of associating values with symbols happens
 *  in this code.  An environment is (essentially) a list of
 *  environment "frames" of the form
 *
 *	FRAME(envir) = environment frame
 *	ENCLOS(envir) = parent environment
 *
 *  Each frame is a (tagged) list with
 *
 *	TAG(item) = symbol
 *	CAR(item) = value bound to symbol in this frame
 *	CDR(item) = next value on the list
 *
 *  When the value of a symbol is required, the environment is
 *  traversed frame-by-frame until a value is found.
 *
 *  If a value is not found during the traversal, the symbol's
 *  "value" slot is inspected for a value.  This "top-level"
 *  environment is where system functions and variables reside.
 *
 */

/* R 1.8.0: namespaces are no longer experimental, so the following
 *  are no longer 'experimental options':
 *
 * EXPERIMENTAL_NAMESPACES: When this is defined the variable
 *     R_BaseNamespace holds an environment that has R_GlobalEnv as
 *     its parent.  This environment does not actually contain any
 *     bindings of its own.  Instead, it redirects all fetches and
 *     assignments to the SYMVALUE fields of the base (R_BaseEnv)
 *     environment.  If evaluation occurs in R_BaseNamespace, then
 *     base is searched before R_GlobalEnv.
 *
 * ENVIRONMENT_LOCKING: Locking an environment prevents new bindings
 *     from being created and existing bindings from being removed.
 *
 * FANCY_BINDINGS: This enables binding locking and "active bindings".
 *     When a binding is locked, its value cannot be changed.  It may
 *     still be removed from the environment if the environment is not
 *     locked.
 *
 *     Active bindings contain a function in their value cell.
 *     Getting the value of an active binding calls this function with
 *     no arguments and returns the result.  Assigning to an active
 *     binding calls this function with one argument, the new value.
 *     Active bindings may be useful for mapping external variables,
 *     such as C variables or data base entries, to R variables.  They
 *     may also be useful for making some globals thread-safe.
 *
 *     Bindings are marked as locked or active using bits 14 and 15 in
 *     their gp fields.  Since the save/load code writes out this
 *     field it means the value will be preserved across save/load.
 *     But older versions of R will interpret the entire gp field as
 *     the MISSING field, which may cause confusion.  If we keep this
 *     code, then we will need to make sure that there are no
 *     locked/active bindings in workspaces written for older versions
 *     of R to read.
 *
 * LT */

/** @file envir.cpp
 *
 * Environments: all the action of associating values with symbols
 * happens in this code.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <functional>
#include <iostream>
#include "Defn.h"
#include <R_ext/Callbacks.h>
#include "CXXR/ProvenanceTracker.h"
#include "CXXR/ClosureContext.hpp"

using namespace CXXR;

namespace {
    inline bool FRAME_IS_LOCKED(SEXP e)
    {
	return SEXP_downcast<Environment*>(e)->frame()->isLocked();
    }
}

static SEXP getActiveValue(SEXP fun)
{
    SEXP expr = LCONS(fun, R_NilValue);
    PROTECT(expr);
    expr = eval(expr, R_GlobalEnv);
    UNPROTECT(1);
    return expr;
}

static void setActiveValue(SEXP fun, SEXP val)
{
    SEXP s_quote = install("quote");
    SEXP arg = LCONS(s_quote, CONS(val, R_NilValue));
    SEXP expr = LCONS(fun, CONS(arg, R_NilValue));
    PROTECT(expr);
    eval(expr, R_GlobalEnv);
    UNPROTECT(1);
}

void Frame::Binding::assign(RObject* new_value, Origin origin)
{
    if (isLocked())
	Rf_error(_("cannot change value of locked binding for '%s'"),
		 symbol()->name()->c_str());
    m_origin = origin;
    if (isActive()) {
	setActiveValue(m_value, new_value);
	m_frame->monitorRead(*this);
    } else {
	m_value = new_value;
	m_frame->monitorWrite(*this);
    }
}

RObject* Frame::Binding::value() const
{
    RObject* ans = (isActive() ? getActiveValue(m_value) : m_value);
    m_frame->monitorRead(*this);
    return ans;
}

/* Macro version of isNull for only the test against R_NilValue */
#define ISNULL(x) ((x) == R_NilValue)

/*----------------------------------------------------------------------

  String Hashing

  This is taken from the second edition of the "Dragon Book" by
  Aho, Ullman and Sethi.

*/

/* was extern: used in this file and names.c (for the symbol table).

   This hash function seems to work well enough for symbol tables,
   and hash tables get saved as part of environments so changing it
   is a major decision.
 */
int attribute_hidden R_Newhashpjw(const char *s)
{
    CXXRCONST char *p;
    unsigned h = 0, g;
    for (p = CXXRNOCAST(char *) s; *p; p++) {
	h = (h << 4) + (*p);
	if ((g = h & 0xf0000000) != 0) {
	    h = h ^ (g >> 24);
	    h = h ^ g;
	}
    }
    return h;
}

/*----------------------------------------------------------------------

  R_NewHashedEnv

*/

SEXP R_NewHashedEnv(SEXP enclos, SEXP size)
{
    Environment* enc = SEXP_downcast<Environment*>(enclos);
    GCStackRoot<Frame> frame(CXXR_NEW(StdFrame(asInteger(size))));
    return CXXR_NEW(Environment(enc, frame));
}

/*----------------------------------------------------------------------

  Environments

  The following code implements variable searching for environments.

*/


/*----------------------------------------------------------------------

  InitGlobalEnv

  Create the initial global environment.  The global environment is
  no longer a linked list of environment frames.  Instead it is a
  vector of environments which is searched from beginning to end.

  Note that only the first frame of each of these environments is
  searched.  This is intended to make it possible to implement
  namespaces at some (indeterminate) point in the future.

  We hash the initial environment.  100 is a magic number discovered
  by Ross.  Change it if you feel inclined.

*/

static SEXP R_BaseNamespaceName;

void attribute_hidden InitGlobalEnv()
{
    SET_SYMVALUE(install(".BaseNamespaceEnv"), R_BaseNamespace);
    R_BaseNamespaceName = ScalarString(mkChar("base"));
    R_PreserveObject(R_BaseNamespaceName);
    GCStackRoot<> zero(ScalarInteger(0));
    R_NamespaceRegistry = R_NewHashedEnv(R_NilValue, zero);
    R_PreserveObject(R_NamespaceRegistry);
    defineVar(install("base"), R_BaseNamespace, R_NamespaceRegistry);

#ifdef PROVENANCE_TRACKING
    ProvenanceTracker::setMonitors();
    //ProvenanceTracker::initEnv(static_cast<Environment*>(R_BaseEnv));
    /**** needed to properly initialize the base namespace */
#endif
}


/*----------------------------------------------------------------------

  unbindVar

  Remove a value from an environment. This happens only in the frame
  of the specified environment.

  FIXME ? should this also unbind the symbol value slot when rho is
  R_BaseEnv.
  This is only called from eval.c in applydefine and bcEval
  (and applydefine only works for unhashed environments, so not base).
*/

void attribute_hidden unbindVar(SEXP symbol, SEXP rho)
{
    if (rho == R_BaseNamespace)
	error(_("cannot unbind in the base namespace"));
    if (rho == R_BaseEnv)
	error(_("unbind in the base environment is unimplemented"));

    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    if (env->frame()->erase(sym) && env == Environment::global())
	R_DirtyImage = 1;
}



/*----------------------------------------------------------------------

  findVarLocInFrame

  Look up the location of the value of a symbol in a
  single environment frame.  Almost like findVarInFrame, but
  does not return the value. R_NilValue if not found.

  Callers set *canCache = TRUE or NULL
*/

// canCache is used for 'user databases': currently (2009-01-18)
// unimplemented in CXXR.
static R_varloc_t findVarLocInFrame(SEXP rho, SEXP symbol, Rboolean * /*canCache*/)
{
    if (!rho || rho == R_EmptyEnv)
	return(R_NilValue);

    Environment* env = SEXP_downcast<Environment*>(rho);
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    return env->frame()->binding(sym);
}


/*
  External version and accessor functions. Returned value is cast as
  an opaque pointer to insure it is only used by routines in this
  group.  This allows the implementation to be changed without needing
  to change other files.
*/

R_varloc_t R_findVarLocInFrame(SEXP rho, SEXP symbol)
{
    return findVarLocInFrame(rho, symbol, NULL);
}

SEXP R_GetVarLocValue(R_varloc_t vl)
{
    return vl->value();
}

SEXP R_GetVarLocSymbol(R_varloc_t vl)
{
    return const_cast<Symbol*>(vl->symbol());
}

Rboolean R_GetVarLocMISSING(R_varloc_t vl)
{
    return Rboolean(vl->origin());
}

void R_SetVarLocValue(R_varloc_t vl, SEXP value)
{
    vl->assign(value);
}


/*----------------------------------------------------------------------

  findVarInFrame

  Look up the value of a symbol in a single environment frame.	This
  is the basic building block of all variable lookups.

  It is important that this be as efficient as possible.

  The final argument is usually TRUE and indicates whether the
  lookup is being done in order to get the value (TRUE) or
  simply to check whether there is a value bound to the specified
  symbol in this frame (FALSE).  This is used for get() and exists().
*/

// doGet is used in connection with user databases, currently
// (2009-01-18) unimplemented in CXXR.
SEXP findVarInFrame3(SEXP rho, SEXP symbol, Rboolean /*doGet*/)
{
    if (TYPEOF(rho) == NILSXP)
	error(_("use of NULL environment is defunct"));

    Environment* env = SEXP_downcast<Environment*>(rho);
    Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Frame::Binding* bdg = env->frame()->binding(sym);
    if (bdg)
	return bdg->value();
    // Reproduce the CR behaviour:
    if (sym == Symbol::missingArgument())
	return sym;
    return R_UnboundValue;
}

SEXP findVarInFrame(SEXP rho, SEXP symbol)
{
    return findVarInFrame3(rho, symbol, TRUE);
}


/*----------------------------------------------------------------------

  findVar

  Look up a symbol in an environment.

*/

SEXP findVar(SEXP symbol, SEXP rho)
{
    if (TYPEOF(rho) == NILSXP)
	error(_("use of NULL environment is defunct"));

    if (!isEnvironment(rho))
	error(_("argument to '%s' is not an environment"), "findVar");

    Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = static_cast<Environment*>(rho);
    Frame::Binding* bdg = env->findBinding(sym).second;
    return (bdg ? bdg->value() : R_UnboundValue);
}



/*----------------------------------------------------------------------

  findVar1

  Look up a symbol in an environment.  Ignore any values which are
  not of the specified type.

*/

namespace {
    // Predicate used to test whether a Binding's value is of a
    // specified type.
    class TypeTester : public std::unary_function<RObject*, bool> {
    public:
	TypeTester(SEXPTYPE type)
	    : m_type(type)
	{}

	bool operator()(const RObject* obj);
    private:
	SEXPTYPE m_type;
    };

    bool TypeTester::operator()(const RObject* obj)
    {
	if (m_type == ANYSXP)
	    return true;
	if (!obj)
	    return m_type == NILSXP;
	if (obj->sexptype() == m_type)
	    return true;
	if (m_type == FUNSXP && FunctionBase::isA(obj))
	    return true;
	return false;
    }
}

SEXP attribute_hidden
findVar1(SEXP symbol, SEXP rho, SEXPTYPE mode, int inherits)
{
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    TypeTester typetest(mode);
    std::pair<bool, RObject*> pr = findTestedValue(sym, env, typetest, inherits);
    return (pr.first ? pr.second : R_UnboundValue);
}

/*
 *  ditto, but check *mode* not *type*
 */

namespace {
    // Predicate used to test whether a Binding's value is of a
    // specified mode.
    class ModeTester : public std::unary_function<RObject*, bool> {
    public:
	ModeTester(SEXPTYPE mode);

	bool operator()(const RObject* obj);
    private:
	SEXPTYPE m_mode;
    };

    ModeTester::ModeTester(SEXPTYPE mode)
	: m_mode(mode)
    {
	if (mode == INTSXP) m_mode = REALSXP;
	if (mode == CLOSXP || mode == BUILTINSXP || mode == SPECIALSXP)
	    m_mode = FUNSXP;
    }

    bool ModeTester::operator()(const RObject* obj)
    {
	if (m_mode == ANYSXP)
	    return true;
	if (!obj)
	    return m_mode == NILSXP;
	SEXPTYPE ost = obj->sexptype();
	if (ost == INTSXP)
	    ost = REALSXP;
	if (ost == m_mode)
	    return true;
	if (m_mode == FUNSXP && FunctionBase::isA(obj))
	    return true;
	return false;
    }
}

static SEXP
findVar1mode(SEXP symbol, SEXP rho, SEXPTYPE mode, int inherits,
	     Rboolean doGet)
{
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    // For ANYSXP, we use shortcuts to avoid running active binding
    // functions.  cf. comment to existsVarInFrame() in CR.
    if (mode == ANYSXP) {
	Frame::Binding* bdg;
	if (!inherits)
	    bdg = env->frame()->binding(sym);
	else bdg = env->findBinding(sym).second;
	return bdg ? bdg->value() : R_UnboundValue;
    }
    ModeTester modetest(mode);
    std::pair<bool, RObject*> pr = findTestedValue(sym, env, modetest, inherits);
    return (pr.first ? pr.second : R_UnboundValue);
}


/*
   ddVal:
   a function to take a name and determine if it is of the form
   ..x where x is an integer; if so x is returned otherwise 0 is returned
*/
static int ddVal(SEXP symbol)
{
    Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    return sym->dotDotIndex();
}

/*----------------------------------------------------------------------
  ddfindVar

  This function fetches the variables ..1, ..2, etc from the first
  frame of the environment passed as the second argument to ddfindVar.
  These variables are implicitly defined whenever a ... object is
  created.

  To determine values for the variables we first search for an
  explicit definition of the symbol, them we look for a ... object in
  the frame and then walk through it to find the appropriate values.

  If no value is obtained we return R_UnboundValue.

  It is an error to specify a .. index longer than the length of the
  ... object the value is sought in.

*/

SEXP ddfindVar(SEXP symbol, SEXP rho)
{
    int i;
    SEXP vl;

    /* first look for ... symbol  */
    vl = findVar(R_DotsSymbol, rho);
    i = ddVal(symbol);
    if (vl != R_UnboundValue) {
	if (length(vl) >= i) {
	    vl = nthcdr(vl, i - 1);
	    return(CAR(vl));
	}
	else
	    error(_("The ... list does not contain %d elements"), i);
    }
    else error(_("..%d used in an incorrect context, no ... to look in"), i);

    return R_NilValue;
}



/*----------------------------------------------------------------------

  dynamicFindVar

  This function does a variable lookup, but uses dynamic scoping rules
  rather than the lexical scoping rules used in findVar.

  Return R_UnboundValue if the symbol isn't located and the calling
  function needs to handle the errors.

*/

SEXP dynamicfindVar(SEXP symbol, ClosureContext *cptr)
{
    SEXP vl;
    while (cptr) {
	vl = findVarInFrame3(cptr->workingEnvironment(), symbol, TRUE);
	if (vl != R_UnboundValue) return vl;
	cptr = ClosureContext::innermost(cptr->nextOut());
    }
    return R_UnboundValue;
}



/*----------------------------------------------------------------------

  findFun

  Search for a function in an environment This is a specially modified
  version of findVar which ignores values its finds if they are not
  functions.

 [ NEEDED: This needs to be modified so that a search for an arbitrary mode can
  be made.  Then findVar and findFun could become same function.]

  This could call findVar1.  NB: they behave differently on failure.
*/

SEXP findFun(SEXP symbol, SEXP rho)
{
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    std::pair<Environment*, FunctionBase*> pr = findFunction(sym, env);
    if (pr.first)
	return pr.second;
    error(_("could not find function \"%s\""), sym->name()->c_str());
    /* NOT REACHED */
    return R_UnboundValue;
}


/*----------------------------------------------------------------------

  defineVar

  Assign a value in a specific environment frame.

*/

void defineVar(SEXP symbol, SEXP value, SEXP rho)
{
    /* R_DirtyImage should only be set if assigning to R_GlobalEnv. */
    if (rho == R_GlobalEnv) R_DirtyImage = 1;

    if (rho == R_EmptyEnv)
	error(_("cannot assign values in the empty environment"));

    GCStackRoot<> valrt(value);
    Environment* env = SEXP_downcast<Environment*>(rho);
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Frame::Binding* bdg = env->frame()->obtainBinding(sym);
    bdg->assign(value);
}


/*----------------------------------------------------------------------

    setVar

    Assign a new value to bound symbol.	 Note this does the "inherits"
    case.  I.e. it searches frame-by-frame for a symbol and binds the
    given value to the first symbol encountered.  If no symbol is
    found then a binding is created in the global environment.

    Changed in R 2.4.0 to look in the base environment (previously the
    search stopped befor the base environment, but would (and still
    does) assign into the base namespace if that is on the search and
    the symbol existed there).

*/

void setVar(SEXP symbol, SEXP value, SEXP rho)
{
    Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    std::pair<Environment*, Frame::Binding*> pr = env->findBinding(sym);
    Frame::Binding* bdg = pr.second;
    env = pr.first;
    if (!env) {
	env = Environment::global();
	bdg = env->frame()->obtainBinding(sym);
    }
    if (env == Environment::global())
	R_DirtyImage = 1;
    bdg->assign(value);
}



/*----------------------------------------------------------------------

  gsetVar

  Assignment in the base environment. Here we assign directly into
  the base environment.

*/

void gsetVar(SEXP symbol, SEXP value, SEXP rho)
{
    GCStackRoot<> valrt(value);
    const Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Frame::Binding* bdg = Environment::base()->frame()->obtainBinding(sym);
    bdg->assign(value);
}

/* get environment from a subclass if possible; else return NULL */
#define simple_as_environment(arg) (IS_S4_OBJECT(arg) && (TYPEOF(arg) == S4SXP) ? R_getS4DataSlot(arg, ENVSXP) : R_NilValue)



/*----------------------------------------------------------------------

  do_assign : .Internal(assign(x, value, envir, inherits))

*/
SEXP attribute_hidden do_assign(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP name=R_NilValue, val, aenv;
    int ginherits = 0;
    checkArity(op, args);

    if (!isString(CAR(args)) || length(CAR(args)) == 0)
	error(_("invalid first argument"));
    else {
	if (length(CAR(args)) > 1)
	    warning(_("only the first element is used as variable name"));
	name = install(translateChar(STRING_ELT(CAR(args), 0)));
    }
    PROTECT(val = CADR(args));
    aenv = CADDR(args);
    if (TYPEOF(aenv) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(aenv) != ENVSXP &&
	TYPEOF((aenv = simple_as_environment(aenv))) != ENVSXP)
	error(_("invalid '%s' argument"), "envir");
    ginherits = asLogical(CADDDR(args));
    if (ginherits == NA_LOGICAL)
	error(_("invalid '%s' argument"), "inherits");
    if (ginherits)
	setVar(name, val, aenv);
    else
	defineVar(name, val, aenv);
    UNPROTECT(1);
    return val;
}


/**
 * do_list2env : .Internal(list2env(x, envir))
  */
SEXP attribute_hidden do_list2env(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP x, xnms, envir;
    int n;
    checkArity(op, args);

    if (TYPEOF(CAR(args)) != VECSXP)
	error(_("first argument must be a named list"));
    x = CAR(args);
    n = LENGTH(x);
    xnms = getAttrib(x, R_NamesSymbol);
    if (TYPEOF(xnms) != STRSXP || LENGTH(xnms) != n)
	error(_("names(x) must be a character vector of the same length as x"));
    envir = CADR(args);
    if (TYPEOF(envir) != ENVSXP)
	error(_("'envir' argument must be an environment"));

    for(int i = 0; i < LENGTH(x) ; i++) {
	SEXP name = install(translateChar(STRING_ELT(xnms, i)));
	defineVar(name, VECTOR_ELT(x, i), envir);
    }

    return envir;
}


/*----------------------------------------------------------------------

  do_remove

  There are three arguments to do_remove; a list of names to remove,
  an optional environment (if missing set it to R_GlobalEnv) and
  inherits, a logical indicating whether to look in the parent env if
  a symbol is not found in the supplied env.  This is ignored if
  environment is not specified.

*/

static int RemoveVariable(SEXP name, SEXP env)
{
    if (env == R_BaseNamespace)
	error(_("cannot remove variables from base namespace"));
    if (env == R_BaseEnv)
	error(_("cannot remove variables from the base environment"));
    if (env == R_EmptyEnv)
	error(_("cannot remove variables from the empty environment"));
    if (FRAME_IS_LOCKED(env))
	error(_("cannot remove bindings from a locked environment"));

    Environment* envir = SEXP_downcast<Environment*>(env);
    const Symbol* sym = SEXP_downcast<Symbol*>(name);
    bool found = envir->frame()->erase(sym);
    if (found && env == R_GlobalEnv)
	R_DirtyImage = 1;
    return found;
}

SEXP attribute_hidden do_remove(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    /* .Internal(remove(list, envir, inherits)) */

    SEXP name, envarg, tsym, tenv;
    int ginherits = 0;
    int done, i;
    checkArity(op, args);

    name = CAR(args);
    if (!isString(name))
	error(_("invalid first argument"));
    args = CDR(args);

    envarg = CAR(args);
    if (TYPEOF(envarg) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(envarg) != ENVSXP &&
	TYPEOF((envarg = simple_as_environment(envarg))) != ENVSXP)
	error(_("invalid '%s' argument"), "envir");
    args = CDR(args);

    ginherits = asLogical(CAR(args));
    if (ginherits == NA_LOGICAL)
	error(_("invalid '%s' argument"), "inherits");

    for (i = 0; i < LENGTH(name); i++) {
	done = 0;
	tsym = install(translateChar(STRING_ELT(name, i)));
	tenv = envarg;
	while (tenv != R_EmptyEnv) {
	    done = RemoveVariable(tsym, tenv);
	    if (done || !ginherits)
		break;
	    tenv = CDR(tenv);
	}
	if (!done)
	    warning(_("object '%s' not found"),
		    CHAR(PRINTNAME(tsym)));
    }
    return R_NilValue;
}


/*----------------------------------------------------------------------

  do_get

  This function returns the SEXP associated with the character
  argument.  It needs the environment of the calling function as a
  default.

      get(x, envir, mode, inherits)
      exists(x, envir, mode, inherits)

*/


SEXP attribute_hidden do_get(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP rval, genv, t1 = R_NilValue;
    SEXPTYPE gmode;
    int ginherits = 0, where;
    checkArity(op, args);

    /* The first arg is the object name */
    /* It must be present and a non-empty string */

    if (!isValidStringF(CAR(args)))
	error(_("invalid first argument"));
    else
	t1 = install(translateChar(STRING_ELT(CAR(args), 0)));

    /* envir :	originally, the "where=" argument */

    if (TYPEOF(CADR(args)) == REALSXP || TYPEOF(CADR(args)) == INTSXP) {
	where = asInteger(CADR(args));
	genv = R_sysframe(where, ClosureContext::innermost());
    }
    else if (TYPEOF(CADR(args)) == NILSXP) {
	error(_("use of NULL environment is defunct"));
	genv = R_NilValue;  /* -Wall */
    }
    else if (TYPEOF(CADR(args)) == ENVSXP)
	genv = CADR(args);
    else if(TYPEOF((genv = simple_as_environment(CADR(args)))) != ENVSXP) {
	error(_("invalid '%s' argument"), "envir");
	genv = R_NilValue;  /* -Wall */
    }

    /* mode :  The mode of the object being sought */

    /* as from R 1.2.0, this is the *mode*, not the *typeof* aka
       storage.mode.
    */

    if (isString(CADDR(args))) {
	if (!strcmp(CHAR(STRING_ELT(CAR(CDDR(args)), 0)), "function")) /* ASCII */
	    gmode = FUNSXP;
	else
	    gmode = str2type(CHAR(STRING_ELT(CAR(CDDR(args)), 0))); /* ASCII */
    } else {
	error(_("invalid '%s' argument"), "mode");
	gmode = FUNSXP;/* -Wall */
    }

    ginherits = asLogical(CADDDR(args));
    if (ginherits == NA_LOGICAL)
	error(_("invalid '%s' argument"), "inherits");

    /* Search for the object */
    rval = findVar1mode(t1, genv, gmode, ginherits, CXXRCONSTRUCT(Rboolean, PRIMVAL(op)));

    if (PRIMVAL(op)) { /* have get(.) */
	if (rval == R_MissingArg)
	    error(_("argument \"%s\" is missing, with no default"),
		  CHAR(PRINTNAME(t1)));
	if (rval == R_UnboundValue) {
	    if (gmode == ANYSXP)
		error(_("object '%s' not found"),
		      CHAR(PRINTNAME(t1)));
	    else
		error(_("object '%s' of mode '%s' was not found"),
		      CHAR(PRINTNAME(t1)),
		      CHAR(STRING_ELT(CAR(CDDR(args)), 0))); /* ASCII */
	}

	/* We need to evaluate if it is a promise */
	if (TYPEOF(rval) == PROMSXP)
	    rval = eval(rval, genv);

	if (!ISNULL(rval) && NAMED(rval) == 0)
	    SET_NAMED(rval, 1);
	return rval;
    }
    else { /* exists(.) */
	if (rval == R_UnboundValue)
	    ginherits = 0;
	else
	    ginherits = 1;
	return ScalarLogical(ginherits);
    }
}

static SEXP gfind(const char *name, SEXP env, SEXPTYPE mode,
		  SEXP ifnotfound, int inherits, SEXP enclos)
{
    SEXP rval, t1, R_fcall, var;

    t1 = install(name);

    /* Search for the object - last arg is 1 to 'get' */
    rval = findVar1mode(t1, env, mode, inherits, CXXRTRUE);

    if (rval == R_UnboundValue) {
	if( isFunction(ifnotfound) ) {
	    PROTECT(var = mkString(name));
	    PROTECT(R_fcall = LCONS(ifnotfound, CONS(var, R_NilValue)));
	    rval = eval(R_fcall, enclos);
	    UNPROTECT(2);
	} else
	    rval = ifnotfound;
    }

    /* We need to evaluate if it is a promise */
    if (TYPEOF(rval) == PROMSXP) rval = eval(rval, env);
    if (!ISNULL(rval) && NAMED(rval) == 0) SET_NAMED(rval, 1);
    return rval;
}


/** mget(): get multiple values from an environment
 *
 * .Internal(mget(x, envir, mode, ifnotfound, inherits))
 *
 * @return  a list of the same length as x, a character vector (of names).
 */
SEXP attribute_hidden do_mget(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP ans, env, x, mode, ifnotfound, ifnfnd;
    SEXPTYPE gmode; /* is unsigned int */
    int ginherits = 0, nvals, nmode, nifnfnd, i;

    checkArity(op, args);

    x = CAR(args);

    nvals = length(x);

    /* The first arg is the object name */
    /* It must be present and a string */
    if (!isString(x) )
	error(_("invalid first argument"));
    for(i = 0; i < nvals; i++)
	if( isNull(STRING_ELT(x, i)) || !CHAR(STRING_ELT(x, 0))[0] )
	    error(_("invalid name in position %d"), i+1);

    /* FIXME: should we install them all?) */

    env = CADR(args);
    if (ISNULL(env)) {
	error(_("use of NULL environment is defunct"));
    } else if( !isEnvironment(env) )
	error(_("second argument must be an environment"));

    mode = CADDR(args);
    nmode = length(mode);
    if( !isString(mode) )
	error(_("invalid '%s' argument"), "mode");

    if( nmode != nvals && nmode != 1 )
	error(_("wrong length for '%s' argument"), "mode");

    PROTECT(ifnotfound = coerceVector(CADDDR(args), VECSXP));
    nifnfnd = length(ifnotfound);
    if( !isVector(ifnotfound) )
	error(_("invalid '%s' argument"), "ifnotfound");

    if( nifnfnd != nvals && nifnfnd != 1 )
	error(_("wrong length for '%s' argument"), "ifnotfound");

    ginherits = asLogical(CAD4R(args));
    if (ginherits == NA_LOGICAL)
	error(_("invalid '%s' argument"), "inherits");

    PROTECT(ans = allocVector(VECSXP, nvals));

    /* now for each element of x, we look for it, using the inherits,
       etc */

    for(i = 0; i < nvals; i++) {
	if (isString(mode)) { /* ASCII */
	    if (!strcmp(CHAR(STRING_ELT(CAR(CDDR(args)), i % nmode )), "function"))
		gmode = FUNSXP;
	    else
		gmode = str2type(CHAR(STRING_ELT(CAR(CDDR(args)), i % nmode )));
	} else {
	    error(_("invalid '%s' argument"), "mode");
	    gmode = FUNSXP; /* -Wall */
	}

	/* is the mode provided one of the real modes? */
	if( gmode == SEXPTYPE( (-1)))
	    error(_("invalid '%s' argument"), "mode");


	if( TYPEOF(ifnotfound) != VECSXP )
	    error(_("invalid '%s' argument"), "ifnotfound");
	if( nifnfnd == 1 ) /* length has been checked to be 1 or nvals. */
	    ifnfnd = VECTOR_ELT(ifnotfound, 0);
	else
	    ifnfnd = VECTOR_ELT(ifnotfound, i);

	SET_VECTOR_ELT(ans, i,
		       gfind(translateChar(STRING_ELT(x,i % nvals)), env,
			     gmode, ifnfnd, ginherits, rho));
    }

    setAttrib(ans, R_NamesSymbol, duplicate(x));
    UNPROTECT(2);
    return(ans);
}

/*----------------------------------------------------------------------

  do_missing

  This function tests whether the symbol passed as its first argument
  is a missing argument to the current closure.  rho is the
  environment that missing was called from.

  R_isMissing is called on the not-yet-evaluated value of an argument,
  if this is a symbol, as it could be a missing argument that has been
  passed down.  So 'symbol' is the promise value, and 'rho' its
  evaluation argument.

  It is also called in arithmetic.c. for e.g. do_log
*/

int attribute_hidden
R_isMissing(SEXP symbol, SEXP rho)
{
    if (!rho)
	return 0;
    Symbol* sym = SEXP_downcast<Symbol*>(symbol);
    Environment* env = SEXP_downcast<Environment*>(rho);
    return isMissingArgument(sym, env->frame());
}

/* this is primitive and a SPECIALSXP */
SEXP attribute_hidden do_missing(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    int ddv=0;
    SEXP sym, s;

    GCStackRoot<> rval;
    GCStackRoot<> t;  // Binding defined in PairList form

    checkArity(op, args);
    check1arg(args, call, "x");
    s = sym = CAR(args);
    if( isString(sym) && length(sym)==1 )
	s = sym = install(translateChar(STRING_ELT(CAR(args), 0)));
    if (!isSymbol(sym))
	errorcall(call, _("invalid use of 'missing'"));

    if (DDVAL(sym)) {
	ddv = ddVal(sym);
	sym = R_DotsSymbol;
    }
    rval = allocVector(LGLSXP,1);

    Frame::Binding* bdg = findVarLocInFrame(rho, sym, NULL);
    t = (bdg ? bdg->asPairList() : 0);
    if (t != R_NilValue) {
	if (DDVAL(s)) {
	    if (length(CAR(t)) < ddv  || CAR(t) == R_MissingArg) {
		LOGICAL(rval)[0] = 1;
		return rval;
	    }
	    else
		t = nthcdr(CAR(t), ddv-1);
	}
	if (MISSING(t) || CAR(t) == R_MissingArg) {
	    LOGICAL(rval)[0] = 1;
	    return rval;
	}
	else goto havebinding;
    }
    else  /* it wasn't an argument to the function */
	errorcall(call, _("'missing' can only be used for arguments"));

 havebinding:

    t = CAR(t);
    if (TYPEOF(t) != PROMSXP) {
	LOGICAL(rval)[0] = 0;
	return rval;
    }

    if (!isSymbol(PREXPR(t))) LOGICAL(rval)[0] = 0;
    else LOGICAL(rval)[0] = R_isMissing(PREXPR(t), PRENV(t));
    return rval;
}

/*----------------------------------------------------------------------

  do_globalenv

  Returns the current global environment.

*/


SEXP attribute_hidden do_globalenv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return R_GlobalEnv;
}

/*----------------------------------------------------------------------

  do_baseenv

  Returns the current base environment.

*/


SEXP attribute_hidden do_baseenv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return R_BaseEnv;
}

/*----------------------------------------------------------------------

  do_emptyenv

  Returns the current empty environment.

*/


SEXP attribute_hidden do_emptyenv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return R_EmptyEnv;
}


/*----------------------------------------------------------------------

  do_attach

  To attach a list we make up an environment and insert components
  of the list in as the values of this env and install the tags from
  the list as the names.

*/

SEXP attribute_hidden do_attach(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP name, x;
    int pos;
    GCStackRoot<Environment> newenv;

    checkArity(op, args);

    pos = asInteger(CADR(args));
    if (pos == NA_INTEGER)
	error(_("'pos' must be an integer"));
    name = CADDR(args);
    if (!isValidStringF(name))
	error(_("invalid '%s' argument"), "name");

    if (isNewList(CAR(args))) {
	SETCAR(args, VectorToPairList(CAR(args)));

	for (x = CAR(args); x != R_NilValue; x = CDR(x))
	    if (TAG(x) == R_NilValue)
		error(_("all elements of a list must be named"));
	GCStackRoot<Frame> frame(CXXR_NEW(StdFrame));
	newenv = CXXR_NEW(Environment(0, frame));
	GCStackRoot<PairList> dupcar(static_cast<PairList*>(duplicate(CAR(args))));
	frameReadPairList(newenv->frame(), dupcar);
    } else if (isEnvironment(CAR(args))) {
	SEXP p, loadenv = CAR(args);

	GCStackRoot<Frame> frame(CXXR_NEW(StdFrame));
	newenv = CXXR_NEW(Environment(0, frame));
	GCStackRoot<> framelist(FRAME(loadenv));
	for(p = framelist; p != R_NilValue; p = CDR(p))
	    defineVar(TAG(p), duplicate(CAR(p)), newenv);
    } else {
	error(_("'attach' only works for lists, data frames and environments"));
	newenv = R_NilValue; /* -Wall */
    }

    setAttrib(newenv, install("name"), name);

    // Interpolate the new environment into the chain of enclosing environments:
    {
	Environment* anchor = Environment::global();
	while (anchor->enclosingEnvironment() != Environment::base()
	       && pos > 2) {
	    anchor = anchor->enclosingEnvironment();
	    --pos;
	}
	newenv->slotBehind(anchor);
    }
    return newenv;
}



/*----------------------------------------------------------------------

  do_detach

  detach the specified environment.  Detachment only takes place by
  position.

*/

SEXP attribute_hidden do_detach(SEXP call, SEXP op, SEXP args, SEXP env)
{
    GCStackRoot<> s;
    SEXP t;
    int pos, n;

    checkArity(op, args);
    pos = asInteger(CAR(args));

    for (n = 2, t = ENCLOS(R_GlobalEnv); t != R_BaseEnv; t = ENCLOS(t))
	n++;

    if (pos == n) /* n is the length of the search list */
	error(_("detaching \"package:base\" is not allowed"));

    for (t = R_GlobalEnv ; ENCLOS(t) != R_BaseEnv && pos > 2 ; t = ENCLOS(t))
	pos--;
    if (pos != 2) {
	error(_("invalid '%s' argument"), "pos");
	s = t;	/* for -Wall */
    }
    else {
	Environment* tenv = static_cast<Environment*>(t);
	s = tenv->enclosingEnvironment();
	tenv->skipEnclosing();
    }
    return s;
}



/*----------------------------------------------------------------------

  do_search

  Print out the current search path.

*/

SEXP attribute_hidden do_search(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans, name, t;
    int i, n;

    checkArity(op, args);
    n = 2;
    for (t = ENCLOS(R_GlobalEnv); t != R_BaseEnv ; t = ENCLOS(t))
	n++;
    PROTECT(ans = allocVector(STRSXP, n));
    /* TODO - what should the name of this be? */
    SET_STRING_ELT(ans, 0, mkChar(".GlobalEnv"));
    SET_STRING_ELT(ans, n-1, mkChar("package:base"));
    i = 1;
    for (t = ENCLOS(R_GlobalEnv); t != R_BaseEnv ; t = ENCLOS(t)) {
	name = getAttrib(t, R_NameSymbol);
	if (!isString(name) || length(name) < 1)
	    SET_STRING_ELT(ans, i, mkChar("(unknown)"));
	else
	    SET_STRING_ELT(ans, i, STRING_ELT(name, 0));
	i++;
    }
    UNPROTECT(1);
    return ans;
}


/*----------------------------------------------------------------------

  do_ls

  This code implements the functionality of the "ls" and "objects"
  functions.  [ ls(envir, all.names) ]

*/

static int FrameSize(SEXP frame, int all)
{
    int count = 0;
    while (frame != R_NilValue) {
	if ((all || CHAR(PRINTNAME(TAG(frame)))[0] != '.') &&
				      CAR(frame) != R_UnboundValue)
	    count += 1;
	frame = CDR(frame);
    }
    return count;
}

static void FrameNames(SEXP frame, int all, SEXP names, int *indx)
{
    while (frame != R_NilValue) {
	if ((all || CHAR(PRINTNAME(TAG(frame)))[0] != '.') &&
				      CAR(frame) != R_UnboundValue) {
	    SET_STRING_ELT(names, *indx, PRINTNAME(TAG(frame)));
	    (*indx)++;
	}
	frame = CDR(frame);
    }
}

static void FrameValues(SEXP frame, int all, SEXP values, int *indx)
{
    while (frame != R_NilValue) {
	if ((all || CHAR(PRINTNAME(TAG(frame)))[0] != '.') &&
				      CAR(frame) != R_UnboundValue) {
	    SEXP value = CAR(frame);
	    if (TYPEOF(value) == PROMSXP) {
		PROTECT(value);
		value = eval(value, R_GlobalEnv);
		UNPROTECT(1);
	    }
	    SET_VECTOR_ELT(values, *indx, duplicate(value));
	    (*indx)++;
	}
	frame = CDR(frame);
    }
}

static bool BuiltinTest(const Symbol* sym, bool all, bool intern)
{
    if (intern && DotInternalTable::get(sym))
	return true;
    if ((all || sym->name()->c_str()[0] != '.')
	&& SYMVALUE(const_cast<Symbol*>(sym)) != R_UnboundValue)
	return true;
    return false;
}

static int BuiltinSize(int all, int intern)
{
    int count = 0;
    for (Symbol::const_iterator it = Symbol::begin();
	 it != Symbol::end(); ++it) {
	const Symbol* sym = *it;
	if (BuiltinTest(sym, all, intern))
	    ++count;
    }
    return count;
}

static void
BuiltinNames(int all, int intern, SEXP names, int *indx)
{
    StringVector* sv = SEXP_downcast<StringVector*>(names);
    for (Symbol::const_iterator it = Symbol::begin();
	 it != Symbol::end(); ++it) {
	const Symbol* sym = *it;
	if (BuiltinTest(sym, all, intern))
	    (*sv)[(*indx)++] = const_cast<String*>(sym->name());
    }
}

SEXP attribute_hidden do_ls(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP env;
    int all;
    checkArity(op, args);

    env = CAR(args);

    all = asLogical(CADR(args));
    if (all == NA_LOGICAL) all = 0;

    return R_lsInternal(env, CXXRCONSTRUCT(Rboolean, all));
}

/* takes a *list* of environments and a boolean indicating whether to get all
   names */
SEXP R_lsInternal(SEXP env, Rboolean all)
{
    if (!isEnvironment(env) &&
	!isEnvironment(env = simple_as_environment(env)))
	error(_("invalid '%s' argument"), "envir");
    const Environment* envir = static_cast<Environment*>(env);
    std::vector<const Symbol*> syms = envir->frame()->symbols(all);
    std::size_t sz = syms.size();
    GCStackRoot<StringVector> ans(CXXR_NEW(StringVector(sz)));
    for (unsigned int i = 0; i < sz; ++i)
	(*ans)[i] = const_cast<String*>(syms[i]->name());
    sortVector(ans, FALSE);
    return ans;
}

/* transform an environment into a named list */

SEXP attribute_hidden do_env2list(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP env, ans, names;
    int k, all;

    checkArity(op, args);

    env = CAR(args);
    if (ISNULL(env))
	error(_("use of NULL environment is defunct"));
    if( !isEnvironment(env) ) {
        SEXP xdata;
	if( IS_S4_OBJECT(env) && TYPEOF(env) == S4SXP &&
	    (xdata = R_getS4DataSlot(env, ENVSXP)) != R_NilValue)
	    env = xdata;
	else
	    error(_("argument must be an environment"));
    }

    GCStackRoot<> framelist(FRAME(env));

    all = asLogical(CADR(args)); /* all.names = TRUE/FALSE */
    if (all == NA_LOGICAL) all = 0;

    k = FrameSize(framelist, all);

    PROTECT(names = allocVector(STRSXP, k));
    PROTECT(ans = allocVector(VECSXP, k));

    k = 0;
    FrameValues(framelist, all, ans, &k);

    k = 0;
    FrameNames(framelist, all, names, &k);

    setAttrib(ans, R_NamesSymbol, names);
    UNPROTECT(2);
    return(ans);
}

/*
 * apply a function to all objects in an environment and return the
 * results in a list.
 * Equivalent to lapply(as.list(env, all.names=all.names), FUN, ...)
 */
/* This is a special .Internal */
SEXP attribute_hidden do_eapply(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP env, ans, R_fcall, FUN, tmp, tmp2, ind;
    int i, k, k2;
    int /* boolean */ all, useNms;

    checkArity(op, args);

    env = eval(CAR(args), rho);
    if (ISNULL(env))
	error(_("use of NULL environment is defunct"));
    if( !isEnvironment(env) )
	error(_("argument must be an environment"));

    FUN = CADR(args);
    if (!isSymbol(FUN))
	error(_("arguments must be symbolic"));

    /* 'all.names' : */
    all = asLogical(eval(CADDR(args), rho));
    if (all == NA_LOGICAL) all = 0;

    /* 'USE.NAMES' : */
    useNms = asLogical(eval(CADDDR(args), rho));
    if (useNms == NA_LOGICAL) useNms = 0;

    GCStackRoot<> framelist(FRAME(env));

    k = FrameSize(framelist, all);

    PROTECT(ans  = allocVector(VECSXP, k));
    PROTECT(tmp2 = allocVector(VECSXP, k));

    k2 = 0;
    FrameValues(framelist, all, tmp2, &k2);

    PROTECT(ind = allocVector(INTSXP, 1));
    /* tmp :=  `[`(<elist>, i) */
    PROTECT(tmp = LCONS(R_Bracket2Symbol,
			CONS(tmp2, CONS(ind, R_NilValue))));
    /* fcall :=  <FUN>( tmp, ... ) */
    PROTECT(R_fcall = LCONS(FUN, CONS(tmp, CONS(R_DotsSymbol, R_NilValue))));

    for(i = 0; i < k2; i++) {
	INTEGER(ind)[0] = i+1;
	SET_VECTOR_ELT(ans, i, eval(R_fcall, rho));
    }

    if (useNms) {
	SEXP names;
	PROTECT(names = allocVector(STRSXP, k));
	k = 0;
	FrameNames(framelist, all, names, &k);

	setAttrib(ans, R_NamesSymbol, names);
	UNPROTECT(1);
    }
    UNPROTECT(5);
    return(ans);
}

int envlength(SEXP rho)
{
    const Environment* env = SEXP_downcast<Environment*>(rho);
    return env->frame()->size();
}

/*----------------------------------------------------------------------

  do_builtins

  Return the names of all the built in functions.  These are fetched
  directly from the symbol table.

*/

SEXP attribute_hidden do_builtins(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP ans;
    int intern, nelts;
    checkArity(op, args);
    intern = asLogical(CAR(args));
    if (intern == NA_INTEGER) intern = 0;
    nelts = BuiltinSize(1, intern);
    ans = allocVector(STRSXP, nelts);
    nelts = 0;
    BuiltinNames(1, intern, ans, &nelts);
    sortVector(ans, TRUE);
    return ans;
}


/*----------------------------------------------------------------------

  do_pos2env

  This function returns the environment at a specified position in the
  search path or the environment of the caller of
  pos.to.env (? but pos.to.env is usually used in arg lists and hence
  is evaluated in the calling environment so this is one higher).

  When pos = -1 the environment of the closure that pos2env is
  evaluated in is obtained. Note: this relies on pos.to.env being
  a primitive.

 */
static SEXP pos2env(int pos, SEXP call)
{
    SEXP env;
    ClosureContext *cptr;

    if (pos == NA_INTEGER || pos < -1 || pos == 0) {
	errorcall(call, _("invalid '%s' argument"), "pos");
	env = call;/* just for -Wall */
    }
    else if (pos == -1) {
	/* make sure the context is a funcall */
	cptr = ClosureContext::innermost();
	if( !cptr )
	    errorcall(call, _("no enclosing environment"));

	env = cptr->callEnvironment();
	if (R_GlobalEnv != R_NilValue && env == R_NilValue)
	    errorcall(call, _("invalid '%s' argument"), "pos");
    }
    else {
	for (env = R_GlobalEnv; env != R_EmptyEnv && pos > 1;
	     env = ENCLOS(env))
	    pos--;
	if (pos != 1)
	    errorcall(call, _("invalid '%s' argument"), "pos");
    }
    return env;
}

/* this is primitive */
SEXP attribute_hidden do_pos2env(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP env, pos;
    int i, npos;
    checkArity(op, args);
    check1arg(args, call, "x");

    PROTECT(pos = coerceVector(CAR(args), INTSXP));
    npos = length(pos);
    if (npos <= 0)
	errorcall(call, _("invalid '%s' argument"), "pos");
    PROTECT(env = allocVector(VECSXP, npos));
    for (i = 0; i < npos; i++) {
	SET_VECTOR_ELT(env, i, pos2env(INTEGER(pos)[i], call));
    }
    if (npos == 1) env = VECTOR_ELT(env, 0);
    UNPROTECT(2);
    return env;
}

static SEXP matchEnvir(SEXP call, const char *what)
{
    SEXP t, name;
    if(!strcmp(".GlobalEnv", what))
	return R_GlobalEnv;
    if(!strcmp("package:base", what))
	return R_BaseEnv;
    for (t = ENCLOS(R_GlobalEnv); t != R_EmptyEnv ; t = ENCLOS(t)) {
	name = getAttrib(t, R_NameSymbol);
	if(isString(name) && length(name) > 0 &&
	   !strcmp(translateChar(STRING_ELT(name, 0)), what))
	    return t;
    }
    errorcall(call, _("no item called \"%s\" on the search list"), what);
    return R_NilValue;
}

/* This is primitive */
SEXP attribute_hidden
do_as_environment(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP arg = CAR(args), ans;
    checkArity(op, args);
    check1arg(args, call, "object");
    if(isEnvironment(arg))
	return arg;
    if(isObject(arg) &&
       DispatchOrEval(call, op, "as.environment", args, rho, &ans, 0, 1))
	return ans;
    switch(TYPEOF(arg)) {
    case STRSXP:
	return matchEnvir(call, translateChar(asChar(arg)));
    case REALSXP:
    case INTSXP:
	return do_pos2env(call, op, args, rho);
    case NILSXP:
	errorcall(call,_("using 'as.environment(NULL)' is defunct"));
	return R_BaseEnv;	/* -Wall */
    case S4SXP: {
	/* dispatch was tried above already */
	SEXP dot_xData = R_getS4DataSlot(arg, ENVSXP);
	if(!isEnvironment(dot_xData))
	    errorcall(call, _("S4 object does not extend class \"environment\""));
	else
	    return(dot_xData);
    }
    case VECSXP: {
	/* implement as.environment.list() {isObject(.) is false for a list} */
	SEXP call, val;
	PROTECT(call = lang4(install("list2env"), arg,
			     /* envir = */R_NilValue,
			     /* parent = */R_EmptyEnv));
	val = eval(call, rho);
	UNPROTECT(1);
	return val;
    }
    default:
	errorcall(call, _("invalid object for 'as.environment'"));
	return R_NilValue;	/* -Wall */
    }
}

void R_LockEnvironment(SEXP env, Rboolean bindings)
{
    if(IS_S4_OBJECT(env) && (TYPEOF(env) == S4SXP))
	env = R_getS4DataSlot(env, ANYSXP); /* better be an ENVSXP */
    if (env == R_BaseEnv || env == R_BaseNamespace) {
	if (bindings) {
	    Environment::base()->frame()->lockBindings();
	}
#ifdef NOT_YET
	/* causes problems with Matrix */
	LOCK_FRAME(env);
#endif
	return;
    }

    if (TYPEOF(env) != ENVSXP)
	error(_("not an environment"));
    Environment* envir = static_cast<Environment*>(env);
    envir->frame()->lock(bindings);
}

Rboolean R_EnvironmentIsLocked(SEXP env)
{
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    return Rboolean(static_cast<Environment*>(env)->frame()->isLocked());
}

SEXP do_lockEnv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP frame;
    Rboolean bindings;
    checkArity(op, args);
    frame = CAR(args);
    bindings = CXXRCONSTRUCT(Rboolean, asLogical(CADR(args)));
    R_LockEnvironment(frame, bindings);
    return R_NilValue;
}

SEXP attribute_hidden do_envIsLocked(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return ScalarLogical(R_EnvironmentIsLocked(CAR(args)));
}

void R_LockBinding(SEXP sym, SEXP env)
{
    if (TYPEOF(sym) != SYMSXP)
	error(_("not a symbol"));
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    const Symbol* symbol = static_cast<Symbol*>(sym);
    Environment* envir = static_cast<Environment*>(env);
    Frame::Binding* binding = envir->frame()->binding(symbol);
    if (!binding)
	error(_("no binding for \"%s\""), symbol->name()->c_str());
    binding->setLocking(true);
}

void R_unLockBinding(SEXP sym, SEXP env)
{
    if (TYPEOF(sym) != SYMSXP)
	error(_("not a symbol"));
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    const Symbol* symbol = static_cast<Symbol*>(sym);
    Environment* envir = static_cast<Environment*>(env);
    Frame::Binding* binding = envir->frame()->binding(symbol);
    if (!binding)
	error(_("no binding for \"%s\""), symbol->name()->c_str());
    binding->setLocking(false);
}

void R_MakeActiveBinding(SEXP sym, SEXP fun, SEXP env)
{
    if (TYPEOF(sym) != SYMSXP)
	error(_("not a symbol"));
    if (! isFunction(fun))
	error(_("not a function"));
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    Environment* envir = static_cast<Environment*>(env);
    const Symbol* symbol = static_cast<Symbol*>(sym);
    Frame::Binding* binding = envir->frame()->obtainBinding(symbol);
    FunctionBase* function = static_cast<FunctionBase*>(fun);
    binding->setFunction(function);
}

Rboolean R_BindingIsLocked(SEXP sym, SEXP env)
{
    if (TYPEOF(sym) != SYMSXP)
	error(_("not a symbol"));
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    const Symbol* symbol = static_cast<Symbol*>(sym);
    Environment* envir = static_cast<Environment*>(env);
    Frame::Binding* binding = envir->frame()->binding(symbol);
    if (!binding)
	error(_("no binding for \"%s\""), symbol->name()->c_str());
    return Rboolean(binding->isLocked());
}

Rboolean R_BindingIsActive(SEXP sym, SEXP env)
{
    if (TYPEOF(sym) != SYMSXP)
	error(_("not a symbol"));
    if (TYPEOF(env) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(env) != ENVSXP &&
	TYPEOF((env = simple_as_environment(env))) != ENVSXP)
	error(_("not an environment"));
    Frame::Binding* binding = findVarLocInFrame(env, sym, NULL);
    if (!binding)
	error(_("no binding for \"%s\""), CHAR(PRINTNAME(sym)));
    return Rboolean(binding->isActive());
}

Rboolean R_HasFancyBindings(SEXP rho)
{
    SEXP frame;

    for (frame = FRAME(rho); frame != R_NilValue; frame = CDR(frame))
	if (IS_ACTIVE_BINDING(frame) || BINDING_IS_LOCKED(frame))
	    return TRUE;
    return FALSE;
}

SEXP attribute_hidden do_lockBnd(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP sym, env;
    checkArity(op, args);
    sym = CAR(args);
    env = CADR(args);
    switch(PRIMVAL(op)) {
    case 0:
	R_LockBinding(sym, env);
	break;
    case 1:
	R_unLockBinding(sym, env);
	break;
    default:
	error(_("unknown op"));
    }
    return R_NilValue;
}

SEXP attribute_hidden do_bndIsLocked(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP sym, env;
    checkArity(op, args);
    sym = CAR(args);
    env = CADR(args);
    return ScalarLogical(R_BindingIsLocked(sym, env));
}

SEXP attribute_hidden do_mkActiveBnd(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP sym, fun, env;
    checkArity(op, args);
    sym = CAR(args);
    fun = CADR(args);
    env = CADDR(args);
    R_MakeActiveBinding(sym, fun, env);
    return R_NilValue;
}

SEXP attribute_hidden do_bndIsActive(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP sym, env;
    checkArity(op, args);
    sym = CAR(args);
    env = CADR(args);
    return ScalarLogical(R_BindingIsActive(sym, env));
}

/* This is a .Internal with no wrapper, currently unused in base R */
SEXP attribute_hidden do_mkUnbound(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP sym;
    checkArity(op, args);
    sym = CAR(args);

    if (TYPEOF(sym) != SYMSXP) error(_("not a symbol"));
    /* This does not allow active bindings to be unbound */
    if (R_BindingIsLocked(sym, R_BaseEnv))
	error(_("cannot unbind a locked binding"));
    if (R_BindingIsActive(sym, R_BaseEnv))
	error(_("cannot unbind an active binding"));
    SET_SYMVALUE(sym, R_UnboundValue);
    return R_NilValue;
}

Rboolean R_IsPackageEnv(SEXP rho)
{
    if (TYPEOF(rho) == ENVSXP) {
	SEXP name = getAttrib(rho, R_NameSymbol);
	CXXRCONST char *packprefix = "package:";
	int pplen = strlen(packprefix);
	if(isString(name) && length(name) > 0 &&
	   ! strncmp(packprefix, CHAR(STRING_ELT(name, 0)), pplen)) /* ASCII */
	    return TRUE;
	else
	    return FALSE;
    }
    else
	return FALSE;
}

SEXP R_PackageEnvName(SEXP rho)
{
    if (TYPEOF(rho) == ENVSXP) {
	SEXP name = getAttrib(rho, R_NameSymbol);
	CXXRCONST char *packprefix = "package:";
	int pplen = strlen(packprefix);
	if(isString(name) && length(name) > 0 &&
	   ! strncmp(packprefix, CHAR(STRING_ELT(name, 0)), pplen)) /* ASCII */
	    return name;
	else
	    return R_NilValue;
    }
    else
	return R_NilValue;
}

const StringVector* Environment::packageName() const
{
    RObject* ans = R_PackageEnvName(const_cast<Environment*>(this));
    return SEXP_downcast<const StringVector*>(ans);
}

SEXP R_FindPackageEnv(SEXP info)
{
    SEXP expr, val;
    PROTECT(info);
    PROTECT(expr = LCONS(install("findPackageEnv"), CONS(info, R_NilValue)));
    val = eval(expr, R_GlobalEnv);
    UNPROTECT(2);
    return val;
}

Environment* Environment::findPackage(const std::string& name)
{
    GCStackRoot<StringVector> pkgsv(asStringVector(name));
    RObject* ans = R_FindPackageEnv(pkgsv);
    return SEXP_downcast<Environment*>(ans);
}

Rboolean R_IsNamespaceEnv(SEXP rho)
{
    if (rho == R_BaseNamespace)
	return TRUE;
    else if (TYPEOF(rho) == ENVSXP) {
	SEXP info = findVarInFrame3(rho, install(".__NAMESPACE__."), TRUE);
	if (info != R_UnboundValue && TYPEOF(info) == ENVSXP) {
	    SEXP spec = findVarInFrame3(info, install("spec"), TRUE);
	    if (spec != R_UnboundValue &&
		TYPEOF(spec) == STRSXP && LENGTH(spec) > 0)
		return TRUE;
	    else
		return FALSE;
	}
	else return FALSE;
    }
    else return FALSE;
}

SEXP attribute_hidden do_isNSEnv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return R_IsNamespaceEnv(CAR(args)) ? mkTrue() : mkFalse();
}

SEXP R_NamespaceEnvSpec(SEXP rho)
{
    /* The namespace spec is a character vector that specifies the
       namespace.  The first element is the namespace name.  The
       second element, if present, is the namespace version.  Further
       elements may be added later. */
    if (rho == R_BaseNamespace)
	return R_BaseNamespaceName;
    else if (TYPEOF(rho) == ENVSXP) {
	SEXP info = findVarInFrame3(rho, install(".__NAMESPACE__."), TRUE);
	if (info != R_UnboundValue && TYPEOF(info) == ENVSXP) {
	    SEXP spec = findVarInFrame3(info, install("spec"), TRUE);
	    if (spec != R_UnboundValue &&
		TYPEOF(spec) == STRSXP && LENGTH(spec) > 0)
		return spec;
	    else
		return R_NilValue;
	}
	else return R_NilValue;
    }
    else return R_NilValue;
}

const StringVector* Environment::namespaceSpec() const
{
    RObject* ans = R_NamespaceEnvSpec(const_cast<Environment*>(this));
    return SEXP_downcast<const StringVector*>(ans);
}

SEXP R_FindNamespace(SEXP info)
{
    SEXP expr, val;
    PROTECT(info);
    PROTECT(expr = LCONS(install("getNamespace"), CONS(info, R_NilValue)));
    val = eval(expr, R_GlobalEnv);
    UNPROTECT(2);
    return val;
}

Environment*  Environment::findNamespace(const StringVector* spec)
{
    RObject* ans = R_FindNamespace(const_cast<StringVector*>(spec));
    return SEXP_downcast<Environment*>(ans);
}

static SEXP checkNSname(SEXP call, SEXP name)
{
    switch (TYPEOF(name)) {
    case SYMSXP:
	break;
    case STRSXP:
	if (LENGTH(name) >= 1) {
	    name = install(translateChar(STRING_ELT(name, 0)));
	    break;
	}
	/* else fall through */
    default:
	errorcall(call, _("bad namespace name"));
    }
    return name;
}

SEXP attribute_hidden do_regNS(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP name, val;
    checkArity(op, args);
    name = checkNSname(call, CAR(args));
    val = CADR(args);
    if (findVarInFrame(R_NamespaceRegistry, name) != R_UnboundValue)
	errorcall(call, _("namespace already registered"));
    defineVar(name, val, R_NamespaceRegistry);
    return R_NilValue;
}

SEXP attribute_hidden do_unregNS(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP name;
    checkArity(op, args);
    name = checkNSname(call, CAR(args));
    if (findVarInFrame(R_NamespaceRegistry, name) == R_UnboundValue)
	errorcall(call, _("namespace not registered"));
    RemoveVariable(name, R_NamespaceRegistry);
    return R_NilValue;
}

SEXP attribute_hidden do_getRegNS(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP name, val;
    checkArity(op, args);
    name = checkNSname(call, CAR(args));
    val = findVarInFrame(R_NamespaceRegistry, name);
    if (val == R_UnboundValue)
	return R_NilValue;
    else
	return val;
}

SEXP attribute_hidden do_getNSRegistry(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    return R_NamespaceRegistry;
}

SEXP attribute_hidden do_importIntoEnv(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    /* This function copies values of variables from one environment
       to another environment, possibly with different names.
       Promises are not forced and active bindings are preserved. */
    SEXP impenv, impnames, expenv, expnames;
    SEXP impsym, expsym, val;
    int i, n;

    GCStackRoot<> binding;  // represented in PairList form.

    checkArity(op, args);

    impenv = CAR(args); args = CDR(args);
    impnames = CAR(args); args = CDR(args);
    expenv = CAR(args); args = CDR(args);
    expnames = CAR(args); args = CDR(args);

    if (TYPEOF(impenv) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(impenv) != ENVSXP && 
	TYPEOF((impenv = simple_as_environment(impenv))) != ENVSXP)
	error(_("bad import environment argument"));
    if (TYPEOF(expenv) == NILSXP)
	error(_("use of NULL environment is defunct"));
    if (TYPEOF(expenv) != ENVSXP &&
	TYPEOF((expenv = simple_as_environment(expenv))) != ENVSXP)
	error(_("bad export environment argument"));
    if (TYPEOF(impnames) != STRSXP || TYPEOF(expnames) != STRSXP)
	error(_("invalid '%s' argument"), "names");
    if (LENGTH(impnames) != LENGTH(expnames))
	error(_("length of import and export names must match"));

    n = LENGTH(impnames);
    for (i = 0; i < n; i++) {
	impsym = install(translateChar(STRING_ELT(impnames, i)));
	expsym = install(translateChar(STRING_ELT(expnames, i)));

	/* find the binding--may be a CONS cell or a symbol */
	SEXP binding = R_NilValue;
	for (SEXP env = expenv;
	     env != R_EmptyEnv && binding == R_NilValue;
	     env = ENCLOS(env)) {
	    Frame::Binding* bdg = findVarLocInFrame(env, expsym, NULL);
	    binding = (bdg ? bdg->asPairList() : 0);
	}
	if (binding == R_NilValue)
	    binding = expsym;

	/* get value of the binding; do not force promises */
	if (TYPEOF(binding) == SYMSXP) {
	    if (SYMVALUE(expsym) == R_UnboundValue)
		error(_("exported symbol '%s' has no value"),
		      CHAR(PRINTNAME(expsym)));
	    val = SYMVALUE(expsym);
	}
	else val = CAR(binding);

	/* import the binding */
	if (IS_ACTIVE_BINDING(binding))
	    R_MakeActiveBinding(impsym, val, impenv);
	else defineVar(impsym, val, impenv);
    }
    return R_NilValue;
}


SEXP attribute_hidden do_envprofile(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    /* Return a list containing profiling information given a hashed
       environment.  For non-hashed environments, this function
       returns R_NilValue.  This seems appropriate since there is no
       way to test whether an environment is hashed at the R level.
    */
    // Unimplemented in CXXR:
    return 0;
}
