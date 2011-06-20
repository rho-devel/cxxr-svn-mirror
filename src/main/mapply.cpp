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
 *  Copyright (C) 2003-7   The R Development Core Team
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
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <basedecl.h>
#include "CXXR/GCStackRoot.hpp"

using namespace CXXR;

SEXP attribute_hidden
do_mapply(SEXP f, SEXP varyingArgs, SEXP constantArgs, SEXP rho)
{

    int m,nc, *lengths, *counters, named, longest=0;

    m = length(varyingArgs);
    nc = length(constantArgs);
    GCStackRoot<> vnames(getAttrib(varyingArgs, R_NamesSymbol));

    named = vnames!=R_NilValue;

    lengths = static_cast<int *>(CXXR_alloc(m, sizeof(int)));
    for(int i = 0; i < m; i++){
	lengths[i] = length(VECTOR_ELT(varyingArgs,i));
	if (lengths[i] > longest) longest=lengths[i];
    }


    counters = static_cast<int *>(CXXR_alloc(m, sizeof(int)));
    for(int i = 0; i < m; counters[i++]=0);

    GCStackRoot<> mindex(allocVector(VECSXP, m));

    /* build a call
       f(dots[[1]][[4]],dots[[2]][[4]],dots[[3]][[4]],d=7)
    */
    // In CXXR:
    // f(dots[[1]][[ nindex[1] ]], dots[[2]][[ nindex[2] ]],...

    GCStackRoot<> fcall;
    if (constantArgs == R_NilValue)
	;
    else if(isVectorList(constantArgs))
	fcall=VectorToPairList(constantArgs);
    else
	error(_("argument 'MoreArgs' of 'mapply' is not a list"));

    Symbol* nindexsym = Symbol::obtain("nindex");
    for(int j = m-1; j >= 0; j--) {
	SET_VECTOR_ELT(mindex, j, ScalarInteger(j+1));

	GCStackRoot<> tmp1(lang3(R_Bracket2Symbol,
				 install("dots"),
				 VECTOR_ELT(mindex, j)));
	GCStackRoot<> tmp1a(lang3(R_BracketSymbol,
				  nindexsym, VECTOR_ELT(mindex, j)));
	GCStackRoot<> tmp2(lang3(R_Bracket2Symbol,
				 tmp1, tmp1a));
	fcall=CONS(tmp2, fcall);

	if (named && CHAR(STRING_ELT(vnames,j))[0]!='\0')
	    SET_TAG(fcall, install(translateChar(STRING_ELT(vnames,j))));

    }
    fcall=LCONS(f, fcall);

    GCStackRoot<IntVector> nindex(CXXR_NEW(IntVector(m)));
    // Set up binding of nindex in rho:
    {
	Environment* env = SEXP_downcast<Environment*>(rho);
	Frame::Binding* bdg = env->frame()->obtainBinding(nindexsym);
	bdg->setValue(nindex);
    }
    
    GCStackRoot<> ans(allocVector(VECSXP, longest));

    for(int i = 0; i < longest; i++) {
	for(int j = 0; j < m; j++) {
	    counters[j] = (++counters[j]>lengths[j]) ? 1 : counters[j];
	    (*nindex)[j] = counters[j];
	}
	SET_VECTOR_ELT(ans, i, eval(fcall, rho));
    }

    for(int j = 0; j < m; j++) {
	if (counters[j] != lengths[j])
	    warning(_("longer argument not a multiple of length of shorter"));
    }

    return(ans);
}
