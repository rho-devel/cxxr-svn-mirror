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

/** @file S11nTextStream.cpp
 *
 * Implementation of class CXXR::S11nTextStream.
 */

#include "CXXR/S11nTextStream.hpp"

#include <locale>
#include <boost/archive/codecvt_null.hpp>
#include <boost/math/special_functions/nonfinite_num_facets.hpp>

using namespace CXXR;

// Refer to the Boost::Math documentation of 'Facets for
// Floating-Point Infinities and NaNs' for the following runes:

void S11nTextStream::prepare(std::istream& istrm)
{
    using namespace std;
    locale default_locale(locale::classic(),
			  new boost::archive::codecvt_null<char>);
    locale nfnum_locale(default_locale,
			new boost::math::nonfinite_num_get<char>);
    istrm.imbue(nfnum_locale);
}

void S11nTextStream::prepare(std::ostream& ostrm)
{
    using namespace std;
    locale default_locale(locale::classic(),
			  new boost::archive::codecvt_null<char>);
    locale nfnum_locale(default_locale,
			new boost::math::nonfinite_num_put<char>);
    ostrm.imbue(nfnum_locale);
}
