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
 *  Copyright (C) 1999-2006   The R Development Core Team.
 *  Andrew Runnalls (C) 2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

/** @file StringVector.h
 * @brief Class CXXR::StringVector and associated C interface.
 *
 * (StringVector implements STRSXP.)
 */

#ifndef STRINGVECTOR_H
#define STRINGVECTOR_H

#include "CXXR/CachedString.h"
#include "CXXR/VectorBase.h"

#ifdef __cplusplus

#include <iostream>
#include "CXXR/HandleVector.hpp"
#include "CXXR/SEXP_downcast.hpp"

namespace CXXR {
    // Template specializations:
    namespace ElementTraits {
	template <>
	inline const RHandle<String>& NA<RHandle<String> >()
	{
	    static RHandle<String> ans(String::NA());
	    return ans;
	}
    }

    template <>
    inline const char* HandleVector<String, STRSXP>::staticTypeName()
    {
	return "character";
    }

    /** @brief Vector of strings.
     */
    class StringVector : public CXXR::HandleVector<String, STRSXP> {
    public:
	/** @brief Create a StringVector.
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.  The elements are initialized with
	 *          CachedString::blank().
	 */
	explicit StringVector(size_t sz)
	    : HandleVector<String, STRSXP>(sz, CachedString::blank())
	{}

	/** @brief Copy constructor.
	 *
	 * Copy the StringVector, using the RHandle copying
	 * semantics.
	 *
	 * @param pattern StringVector to be copied.
	 */
	StringVector(const StringVector& pattern)
	    : HandleVector<String, STRSXP>(pattern)
	{}

	/** @brief Create a StringVector containing a single string.
	 *
	 * This constructor constructs a StringVector containing a
	 * single element, and initializes that element to point to a
	 * specified CachedString.
	 *
	 * @param string Pointer to the CachedString to be represented
	 *          by the single vector element.
	 */
	explicit StringVector(CachedString* string)
	    : HandleVector<String, STRSXP>(1, string)
	{}

	/** @brief Create a StringVector containing a single std::string.
	 *
	 * This constructor constructs a StringVector containing a
	 * single element, and initializes that element to represent
	 * a specified string and encoding.
	 *
	 * @param str The required text of the single vector element.
	 *
	 * @param encoding The required encoding of the single vector
	 *          element.  Only CE_NATIVE, CE_UTF8 or CE_LATIN1 are
	 *          permitted in this context (checked).
	 */
	explicit StringVector(const std::string& str,
			      cetype_t encoding = CE_NATIVE)
	    : HandleVector<String, STRSXP>(1,
					   CachedString::obtain(str, encoding))
	{}

	// Virtual function of RObject:
	StringVector* clone() const;
    private:
	/**
	 * Declared private to ensure that StringVector objects are
	 * allocated only using 'new'.
	 */
	~StringVector() {}
    };

    /** @brief (For debugging.)
     *
     * @note The name and interface of this function may well change.
     */
    void strdump(std::ostream& os, const StringVector& sv, size_t margin = 0);
}  // namespace CXXR

extern "C" {
#endif /* __cplusplus */

    /**
     * @param s Pointer to an RObject.
     * @return TRUE iff the RObject pointed to by \a s is a vector of
     *         strings.
     */
#ifndef __cplusplus
    Rboolean Rf_isString(SEXP s);
#else
    inline Rboolean Rf_isString(SEXP s)
    {
	return Rboolean(s && TYPEOF(s) == STRSXP);
    }
#endif

/** @brief Set element of CXXR::StringVector.
 * 
 * @param x Non-null pointer to a CXXR::StringVector .
 *
 * @param i Index of the required element.  There is no bounds checking.
 *
 * @param v Non-null pointer to CXXR::String representing the new value.
 */
void SET_STRING_ELT(SEXP x, int i, SEXP v);

/**
 * @brief Examine element of a CXXR::StringVector.
 *
 * @param x Non-null pointer to a CXXR::StringVector.  An error is
 *          raised if \a x is not a pointer to a CXXR::StringVector.
 *
 * @param i Index of the required element.  There is no bounds
 *          checking.
 *
 * @return Pointer to extracted \a i 'th element.
 */
#ifndef __cplusplus
SEXP STRING_ELT(SEXP x, int i);
#else
inline SEXP STRING_ELT(SEXP x, int i)
{
    using namespace CXXR;
    return (*SEXP_downcast<StringVector*>(x, false))[i];
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* STRINGVECTOR_H */
