/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-12 Andrew R. Runnalls, subject to such other
 *CXXR copyrights and copyright restrictions as may be stated below.
 *CXXR 
 *CXXR CXXR is not part of the R project, and bugs and other issues should
 *CXXR not be reported via r-bugs or other R project channels; instead refer
 *CXXR to the CXXR website.
 *CXXR */

/*
 *  R : A Computer Language for Statistical Data Analysis

 *  Copyright (C) 1994-9 W. N. Venables and B. D. Ripley
 *  Copyright (C) 2007-2007  The R Development Core Team
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

/* Original (by permission) from
 * MASS/MASS.c by W. N. Venables and B. D. Ripley

 * Find maximum column: designed for probabilities.
 * Uses reservoir sampling to break ties at random.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <R_ext/Arith.h>	/* NA handling */
#include <Rmath.h>		/* fmax2 */
#include <R_ext/Random.h>	/* ..RNGstate */

#include <R_ext/Applic.h>	/* NA handling */

#define RELTOL 1e-5

void R_max_col(double *matrix, int *nr, int *nc, int *maxes, int *ties_meth)
{
    int	  r, c, m, n_r = *nr;
    double a, b, large;
    Rboolean isna,
	used_random = FALSE,
	do_rand = *ties_meth == 1;

    for (r = 0; r < n_r; r++) {
	/* first check row for any NAs and find the largest abs(entry) */
	large = 0.0;
	isna = FALSE;
	for (c = 0; c < *nc; c++) {
	    a = matrix[r + c * n_r];
	    if (ISNAN(a)) { isna = TRUE; break; }
	    if (!R_FINITE(a)) continue;
	    if (do_rand) large = fmax2(large, fabs(a));
	}
	if (isna) { maxes[r] = NA_INTEGER; continue; }

	m = 0;
	a = matrix[r];
	if (do_rand) {
	    double tol = RELTOL * large;
	    int ntie = 1;
	    for (c = 1; c < *nc; c++) {
		b = matrix[r + c * n_r];
		if (b > a + tol) { /* tol could be zero */
		    a = b; m = c;
		    ntie = 1;
		} else if (b >= a - tol) { /* b ~= current max. a */
		    ntie++;
		    if (!used_random) { GetRNGstate(); used_random = TRUE; }
		    if (ntie * unif_rand() < 1.) m = c;
		}
	    }
	} else {
	    if(*ties_meth == 2) /* return the *first* max if there are ties */
		for (c = 1; c < *nc; c++) {
		    b = matrix[r + c * n_r];
		    if (a < b) {
			a = b; m = c;
		    }
		}
	    else if(*ties_meth == 3) /* return the *last* max ... */
		for (c = 1; c < *nc; c++) {
		    b = matrix[r + c * n_r];
		    if (a <= b) {
			a = b; m = c;
		    }
		}
	    else error("invalid 'ties_meth' {should not happen}");
	}
	maxes[r] = m + 1;
    }
    if(used_random) PutRNGstate();
}
