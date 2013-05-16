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

/** @file S11nScope.cpp
 *
 * Implementation of class S11nScope.
 */

#include "CXXR/S11nScope.hpp"

#include <cstdlib>
#include <iostream>

using namespace CXXR;

S11nScope* S11nScope::s_innermost = 0;

void S11nScope::seqError()
{
    using namespace std;
    cerr << "Fatal error:"
	    " S11nScopes must be destroyed in reverse order of creation\n";
    abort();
}
