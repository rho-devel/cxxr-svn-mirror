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

/** @file Test of SETLENGTH().
 */

#include <algorithm>
#include <iostream>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "CXXR/GCStackRoot.hpp"
#include "CXXR/StringVector.h"

using namespace std;
using namespace CXXR;

namespace {
    void show(const StringVector* sv)
    {
	using namespace boost::lambda;
	std::transform(sv->begin(), sv->end(),
		       ostream_iterator<const char*>(cout, "|"),
		       bind(&String::c_str, _1));
	cout << '\n';
    }
}

int main()
{
    GCStackRoot<StringVector> sv(CXXR_NEW(StringVector(4)));
    (*sv)[0] = CachedString::obtain("fee");
    (*sv)[1] = CachedString::obtain("fie");
    (*sv)[2] = CachedString::obtain("fo");
    (*sv)[3] = CachedString::obtain("fum");
    show(sv);
    SETLENGTH(sv, 6);
    show(sv);
    SETLENGTH(sv, 2);
    show(sv);
    SETLENGTH(sv, 1);
    show(sv);
    SETLENGTH(sv, 0);
    show(sv);
    SETLENGTH(sv, 3);
    show(sv);
    (*sv)[1] = CachedString::obtain("foo");
    show(sv);
    return 0;
}
