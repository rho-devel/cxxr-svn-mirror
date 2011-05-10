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
 *  Copyright (C) 1998--2008  The R Development Core Team.
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

/** @file memory.cpp
 *
 * Memory management, garbage collection, and memory profiling.
 */

// For debugging:
#include <iostream>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <R_ext/RS.h> /* for S4 allocation */
#include "CXXR/FunctionContext.hpp"
#include "CXXR/GCManager.hpp"
#include "CXXR/MemoryBank.hpp"

#include <Defn.h>
#include <R_ext/GraphicsEngine.h> /* GEDevDesc, GEgetDevice */
#include <R_ext/Rdynload.h>
#include "Rdynpriv.h"

using namespace CXXR;

#if defined(Win32) && defined(LEA_MALLOC)
/*#include <stddef.h> */
extern void *Rm_malloc(size_t n);
extern void *Rm_calloc(size_t n_elements, size_t element_size);
extern void Rm_free(void * p);
extern void *Rm_realloc(void * p, size_t n);
#define calloc Rm_calloc
#define malloc Rm_malloc
#define realloc Rm_realloc
#define free Rm_free
#endif

/* malloc uses size_t.  We are assuming here that size_t is at least
   as large as unsigned long.  Changed from int at 1.6.0 to (i) allow
   2-4Gb objects on 32-bit system and (ii) objects limited only by
   length on a 64-bit system.
*/

#define GC_TORTURE

#ifdef GC_TORTURE
# define FORCE_GC !gc_inhibit_torture
#else
# define FORCE_GC 0
#endif

#define GC_PROT(X) {int __t = gc_inhibit_torture; \
	gc_inhibit_torture = 1 ; X ; gc_inhibit_torture = __t;}

/* Miscellaneous Globals. */

/* Debugging Routines. */

#ifdef DEBUG_ADJUST_HEAP
static void DEBUG_ADJUST_HEAP_PRINT(double node_occup, double vect_occup)
{
    R_size_t alloc;
    REprintf("Node occupancy: %.0f%%\nVector occupancy: %.0f%%\n",
	     100.0 * node_occup, 100.0 * vect_occup);
    alloc = MemoryBank::bytesAllocated();
    REprintf("Total allocation: %lu\n", alloc);
    REprintf("Ncells %lu\nVcells %lu\n", R_NSize, R_VSize);
}
#else
#define DEBUG_ADJUST_HEAP_PRINT(node_occup, vect_occup)
#endif /* DEBUG_ADJUST_HEAP */

/* Finalization and Weak References */

SEXP attribute_hidden do_regFinaliz(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    int onexit;

    checkArity(op, args);

    if (TYPEOF(CAR(args)) != ENVSXP && TYPEOF(CAR(args)) != EXTPTRSXP)
	error(_("first argument must be environment or external pointer"));
    if (TYPEOF(CADR(args)) != CLOSXP)
	error(_("second argument must be a function"));

    onexit = asLogical(CADDR(args));
    if(onexit == NA_LOGICAL)
	error(_("third argument must be 'TRUE' or 'FALSE'"));

    R_RegisterFinalizerEx(CAR(args), CADR(args), Rboolean(onexit));
    return R_NilValue;
}


/* The Generational Collector. */

unsigned int GCNode::protectCstructs()
{
    unsigned int protect_count = 0;
#ifdef BYTECODE
    // Bytecode stack:
    {
	SEXP *sp;
	for (sp = R_BCNodeStackBase; sp < R_BCNodeStackTop; sp++) {
	    PROTECT(*sp);
	    ++protect_count;
	}
    }
#endif
    return protect_count;
}

SEXP attribute_hidden do_gctorture(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    int i;
    SEXP old = ScalarLogical(!gc_inhibit_torture);

    checkArity(op, args);
    i = asLogical(CAR(args));
    if (i != NA_LOGICAL)
	gc_inhibit_torture = !i;
    return old;
}

SEXP attribute_hidden do_gcinfo(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    std::ostream* report_os = GCManager::setReporting(0);
    bool want_reporting = asLogical(CAR(args));
    if (want_reporting != NA_LOGICAL)
	GCManager::setReporting(want_reporting ? &std::cerr : 0);
    else
	GCManager::setReporting(report_os);
    return ScalarLogical(report_os != 0);
}

/* reports memory use to profiler in eval.c */

void attribute_hidden get_current_mem(unsigned long *smallvsize,
				    unsigned long *largevsize,
				    unsigned long *nodes)
{
    // All subject to change in CXXR:
    *smallvsize = 0;
    *largevsize = MemoryBank::bytesAllocated()/sizeof(VECREC);
    *nodes = GCNode::numNodes();
    return;
}

SEXP attribute_hidden do_gc(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    checkArity(op, args);
    std::ostream* report_os
	= GCManager::setReporting(asLogical(CAR(args)) ? &std::cerr : 0);
    bool reset_max = asLogical(CADR(args));
    GCManager::gc();
    GCManager::setReporting(report_os);
    /*- now return the [used , gc trigger size] for cells and heap */
    GCStackRoot<> value(allocVector(REALSXP, 6));
    REAL(value)[0] = GCNode::numNodes();
    REAL(value)[1] = NA_REAL;
    REAL(value)[2] = GCManager::maxNodes();
    /* next four are in 0.1MB, rounded up */
    REAL(value)[3] = 0.1*ceil(10. * MemoryBank::bytesAllocated()/Mega);
    REAL(value)[4] = 0.1*ceil(10. * GCManager::triggerLevel()/Mega);
    REAL(value)[5] = 0.1*ceil(10. * GCManager::maxBytes()/Mega);
    if (reset_max) GCManager::resetMaxTallies();
    return value;
}


#ifdef _R_HAVE_TIMING_

/* Use header files! 2007/06/11 arr
// defined in unix/sys-unix.c :
double R_getClockIncrement(void);
void R_getProcTime(double *data);
*/

static double gctimes[5], gcstarttimes[5];
static Rboolean gctime_enabled = FALSE;

SEXP do_gctime(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans;
    if (args == R_NilValue)
	gctime_enabled = TRUE;
    else
	gctime_enabled = Rboolean(asLogical(CAR(args)));
    ans = allocVector(REALSXP, 5);
    REAL(ans)[0] = gctimes[0];
    REAL(ans)[1] = gctimes[1];
    REAL(ans)[2] = gctimes[2];
    REAL(ans)[3] = gctimes[3];
    REAL(ans)[4] = gctimes[4];
    return ans;
}

static void gc_start_timing(void)
{
    if (gctime_enabled)
	R_getProcTime(gcstarttimes);
}

static void gc_end_timing(void)
{
    if (gctime_enabled) {
	double times[5], delta;
	R_getProcTime(times);
	delta = R_getClockIncrement();

	/* add delta to compensate for timer resolution */
	gctimes[0] += times[0] - gcstarttimes[0] + delta;
	gctimes[1] += times[1] - gcstarttimes[1] + delta;
	gctimes[2] += times[2] - gcstarttimes[2] + delta;
	gctimes[3] += times[3] - gcstarttimes[3];
	gctimes[4] += times[4] - gcstarttimes[4];
    }
}
#else /* not _R_HAVE_TIMING_ */
SEXP attribute_hidden do_gctime(SEXP call, SEXP op, SEXP args, SEXP env)
{
    error(_("gc.time() is not implemented on this system"));
    return R_NilValue;		/* -Wall */
}
#endif /* not _R_HAVE_TIMING_ */


/* InitMemory : Initialise the memory to be used in R. */
/* This includes: stack space, node space and vector space */

void InitMemory()
{
#ifdef _R_HAVE_TIMING_
    GCManager::setMonitors(gc_start_timing, gc_end_timing);
#endif
    GCManager::setReporting(R_Verbose ? &std::cerr : 0);
    GCManager::setGCThreshold(R_VSize);

#ifdef BYTECODE
    R_BCNodeStackBase = static_cast<SEXP *>( malloc(R_BCNODESTACKSIZE * sizeof(SEXP)));
    if (R_BCNodeStackBase == NULL)
	R_Suicide("couldn't allocate node stack");
# ifdef BC_INT_STACK
    R_BCIntStackBase =
      (IStackval *) malloc(R_BCINTSTACKSIZE * sizeof(IStackval));
    if (R_BCIntStackBase == NULL)
	R_Suicide("couldn't allocate integer stack");
# endif
    R_BCNodeStackTop = R_BCNodeStackBase;
    R_BCNodeStackEnd = R_BCNodeStackBase + R_BCNODESTACKSIZE;
# ifdef BC_INT_STACK
    R_BCIntStackTop = R_BCIntStackBase;
    R_BCIntStackEnd = R_BCIntStackBase + R_BCINTSTACKSIZE;
# endif
#endif
}


/*----------------------------------------------------------------------

  NewEnvironment

  Create an environment by extending "rho" with a frame obtained by
  pairing the variable names given by the tags on "namelist" with
  the values given by the elements of "valuelist".

  This definition allows the namelist argument to be shorter than the
  valuelist; in this case the remaining values must be named already.
  (This is useful in cases where the entire valuelist is already
  named--namelist can then be R_NilValue.)
*/
SEXP NewEnvironment(SEXP namelist, SEXP valuelist, SEXP rho)
{
    SEXP v = valuelist;
    SEXP n = namelist;
    while (v != R_NilValue && n != R_NilValue) {
	SET_TAG(v, TAG(n));
	v = CDR(v);
	n = CDR(n);
    }
    
    GCStackRoot<> namelistr(namelist);
    GCStackRoot<PairList> namevalr(SEXP_downcast<PairList*>(valuelist));
    GCStackRoot<Environment> rhor(SEXP_downcast<Environment*>(rho));
    // +5 to leave some room for local variables:
    GCStackRoot<Frame> frame(CXXR_NEW(StdFrame(Rf_length(namevalr) + 5)));
    frameReadPairList(frame, namevalr);
    return CXXR_NEW(Environment(rhor, frame));
}

/* Allocate a vector object (and also list-like objects).
*/

SEXP allocVector(SEXPTYPE type, R_len_t length)
{
    SEXP s;

    if (length < 0 ) {
	FunctionContext* ctxt = FunctionContext::innermost();
	errorcall(ctxt ? CXXRCCAST(Expression*, ctxt->call()) : static_cast<RObject*>(0),
		  _("negative length vectors are not allowed"));
    }
    /* number of vector cells to allocate */
    switch (type) {
    case NILSXP:
	return 0;
    case RAWSXP:
	s = new RawVector(length);
	break;
    case CHARSXP:
	error("use of allocVector(CHARSXP ...) is defunct\n");
	break;
    case LGLSXP:
	s = new LogicalVector(length);
	break;
    case INTSXP:
	s = new IntVector(length);
	break;
    case REALSXP:
	s = new RealVector(length);
	break;
    case CPLXSXP:
	s = new ComplexVector(length);
	break;
    case STRSXP:
	s = new StringVector(length);
	break;
    case EXPRSXP:
	s = new ExpressionVector(length);
	break;
    case VECSXP:
	s = new ListVector(length);
	break;
    case LANGSXP:
	{
	    if (length == 0)
		return 0;
	    GCStackRoot<PairList> tl(PairList::make(length - 1));
	    s = new Expression(0, tl);
	    break;
	}
    case LISTSXP:
	return allocList(length);
    default:
	error(_("invalid type/length (%s/%d) in vector allocation"),
	      type2char(type), length);
	return 0;  // -Wall
    }
    return GCNode::expose(s);
}


/* "gc" a mark-sweep or in-place generational garbage collector */

void R_gc(void)
{
    GCManager::gc();
}


#define R_MAX(a,b) (a) < (b) ? (b) : (a)

SEXP attribute_hidden do_memlimits(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans;
    checkArity(op, args);
    PROTECT(ans = allocVector(INTSXP, 2));
    INTEGER(ans)[0] = NA_INTEGER;
    INTEGER(ans)[1] = NA_INTEGER;
    UNPROTECT(1);
    return ans;
}

SEXP attribute_hidden do_memoryprofile(SEXP call, SEXP op, SEXP args, SEXP env)
{
    SEXP ans, nms;
    PROTECT(ans = allocVector(INTSXP, 24));
    PROTECT(nms = allocVector(STRSXP, 24));
    for (int i = 0; i < 24; i++) {
	INTEGER(ans)[i] = 0;
	SET_STRING_ELT(nms, i, type2str(SEXPTYPE(i > LGLSXP? i+2 : i)));
    }
    setAttrib(ans, R_NamesSymbol, nms);
    // Just return a vector of zeroes in CXXR.
    UNPROTECT(2);
    return ans;
}

/* S-like wrappers for calloc, realloc and free that check for error
   conditions */

void *R_chk_calloc(std::size_t nelem, std::size_t elsize)
{
    void *p;
#ifndef HAVE_WORKING_CALLOC
    if(nelem == 0)
	return(NULL);
#endif
    p = calloc(nelem, elsize);
    if(!p) error(_("Calloc could not allocate (%u of %u) memory"),
		 nelem, elsize);
    return(p);
}

void *R_chk_realloc(void *ptr, std::size_t size)
{
    void *p;
    /* Protect against broken realloc */
    if(ptr) p = realloc(ptr, size); else p = malloc(size);
    if(!p) error(_("Realloc could not re-allocate (size %u) memory"), size);
    return(p);
}

void R_chk_free(void *ptr)
{
    /* S-PLUS warns here, but there seems no reason to do so */
    /* if(!ptr) warning("attempt to free NULL pointer by Free"); */
    if(ptr) free(ptr); /* ANSI C says free has no effect on NULL, but
			  better to be safe here */
}


/* External Pointer Objects */

/* Work around casting issues: works where it is needed */
typedef union {void *p; DL_FUNC fn;} fn_ptr;

/* used in package methods */
SEXP R_MakeExternalPtrFn(DL_FUNC p, SEXP tag, SEXP prot)
{
    fn_ptr tmp;
    tmp.fn = p;
    return R_MakeExternalPtr(tmp.p, tag, prot);
}

DL_FUNC R_ExternalPtrAddrFn(SEXP s)
{
    fn_ptr tmp;
    tmp.p =  EXTPTR_PTR(s);
    return tmp.fn;
}


/*******************************************/
/* Non-sampling memory use profiler
   reports all large vector heap
   allocations and all calls to GetNewPage */
/*******************************************/

#ifndef R_MEMORY_PROFILING

SEXP attribute_hidden do_Rprofmem(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    error(_("memory profiling is not available on this system"));
    return R_NilValue; /* not reached */
}

#else
static FILE *R_MemReportingOutfile;

static void R_OutputStackTrace(FILE *file)
{
    int newline = 0;
    Evaluator::Context *cptr;

    for (cptr = Evaluator::Context::innermost(); cptr; cptr = cptr->nextOut()) {
	if (TYPEOF(cptr->call) == LANGSXP) {
	    SEXP fun = CAR(cptr->call);
	    if (!newline) newline = 1;
	    fprintf(file, "\"%s\" ",
		    TYPEOF(fun) == SYMSXP ? CHAR(PRINTNAME(fun)) :
		    "<Anonymous>");
	}
    }
    if (newline) fprintf(file, "\n");
}

static void R_ReportAllocation(R_size_t size)
{
    fprintf(R_MemReportingOutfile, "%ld :", static_cast<unsigned long>(size));
    R_OutputStackTrace(R_MemReportingOutfile);
}

static void R_EndMemReporting()
{
    if(R_MemReportingOutfile != NULL) {
	/* does not fclose always flush? */
	fflush(R_MemReportingOutfile);
	fclose(R_MemReportingOutfile);
	R_MemReportingOutfile=NULL;
    }
    MemoryBank::setMonitor(0);
    return;
}

static void R_InitMemReporting(SEXP filename, int append,
			       R_size_t threshold)
{
    if(R_MemReportingOutfile != NULL) R_EndMemReporting();
    R_MemReportingOutfile = RC_fopen(filename, append ? "a" : "w", TRUE);
    if (R_MemReportingOutfile == NULL)
	error(_("Rprofmem: cannot open output file '%s'"), filename);
    MemoryBank::setMonitor(R_ReportAllocation, threshold);
}

SEXP attribute_hidden do_Rprofmem(SEXP call, SEXP op, SEXP args, SEXP rho)
{
    SEXP filename;
    R_size_t threshold;
    int append_mode;

    checkArity(op, args);
    if (!isString(CAR(args)) || (LENGTH(CAR(args))) != 1)
	error(_("invalid '%s' argument"), "filename");
    append_mode = asLogical(CADR(args));
    filename = STRING_ELT(CAR(args), 0);
    threshold = CXXRCONSTRUCT(R_size_t, REAL(CADDR(args))[0]);
    if (strlen(CHAR(filename)))
	R_InitMemReporting(filename, append_mode, threshold);
    else
	R_EndMemReporting();
    return R_NilValue;
}

#endif /* R_MEMORY_PROFILING */

/* RBufferUtils, moved from deparse.c */

#include "RBufferUtils.h"

attribute_hidden
void *R_AllocStringBuffer(std::size_t blen, R_StringBuffer *buf)
{
    std::size_t blen1, bsize = buf->defaultSize;

    /* for backwards compatibility, probably no longer needed */
    if(blen == std::size_t(-1)) {
	warning("R_AllocStringBuffer(-1) used: please report");
	R_FreeStringBufferL(buf);
	return NULL;
    }

    if(blen * sizeof(char) < buf->bufsize) return buf->data;
    blen1 = blen = (blen + 1) * sizeof(char);
    blen = (blen / bsize) * bsize;
    if(blen < blen1) blen += bsize;

    if(buf->data == NULL) {
	buf->data = static_cast<char *>( malloc(blen));
	buf->data[0] = '\0';
    } else
	buf->data = static_cast<char *>( realloc(buf->data, blen));
    buf->bufsize = blen;
    if(!buf->data) {
	buf->bufsize = 0;
	/* don't translate internal error message */
	error("could not allocate memory (%u Mb) in C function 'R_AllocStringBuffer'",
	      static_cast<unsigned int>( blen)/1024/1024);
    }
    return buf->data;
}

void attribute_hidden
R_FreeStringBuffer(R_StringBuffer *buf)
{
    if (buf->data != NULL) {
	free(buf->data);
	buf->bufsize = 0;
	buf->data = NULL;
    }
}

void attribute_hidden
R_FreeStringBufferL(R_StringBuffer *buf)
{
    if (buf->bufsize > buf->defaultSize) {
	free(buf->data);
	buf->bufsize = 0;
	buf->data = NULL;
    }
}

/* ======== These need direct access to gp field for efficiency ======== */

/* FIXME: consider inlining here */

/* this has NA_STRING = NA_STRING */
int Seql(SEXP a, SEXP b)
{
    /* The only case where pointer comparisons do not suffice is where
      we have two strings in different encodings (which must be
      non-ASCII strings). Note that one of the strings could be marked
      as unknown. */
    if (a == b) return 1;
    /* Leave this to compiler to optimize */
    if (IS_CACHED(a) && IS_CACHED(b) && ENC_KNOWN(a) == ENC_KNOWN(b))
	return 0;
    else {
	const void* vmax = vmaxget();
    	int result = !strcmp(translateCharUTF8(a), translateCharUTF8(b));
    	vmaxset(vmax); /* discard any memory used by translateCharUTF8 */
    	return result;
    }
}


