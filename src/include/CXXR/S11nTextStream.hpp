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

/** @file S11nTextStream.hpp
 *
 * @brief Classes CXXR::S11nTextStream.
 */

#ifndef S11NTEXTSTREAM_HPP
#define S11NTEXTSTREAM_HPP 1

#include <iostream>

namespace CXXR {
    /** @brief Prepare stream for text-based serialization.
     *
     * This class, all of whose members are static, provides
     * facilities for preparing std::stream objects for use with
     * text-based serialization (including XML serialization) with the
     * boost::serialization library.  In particular it ensures that
     * floating-point infinities and NaNs are handled correctly.
     */
    struct S11nTextStream {
	/** @brief Prepare std::istream.
	 *
	 * @param istrm std::istream to be prepared.
	 */
	static void prepare(std::istream& istrm);

	/** @brief Prepare std::ostream.
	 *
	 * @param ostrm std::ostream to be prepared.
	 */
	static void prepare(std::ostream& ostrm);
    };
}

#endif  // S11NTEXTSTREAM_HPP
