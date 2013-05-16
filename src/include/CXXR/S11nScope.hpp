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

/** @file S11nScope.hpp
 *
 * @brief Class S11nScope.
 */

#ifndef S11NSCOPE_HPP
#define S11NSCOPE_HPP 1

#include <boost/utility.hpp>
#include "CXXR/DB.hpp"

namespace CXXR {
    /** @brief Class providing supplementary information for serialization.
     *
     * This class is used to provide supplementary information to
     * control the behaviour of serialization and deserialization, and
     * it particularly important when serialization is directed to a
     * CXXR::DB database (which entails the use of multiple
     * boost::archive objects, including one for each persistent
     * binding).
     *
     * S11nScope objects are intended to be allocated on the processor
     * stack: specifically, the class implementation requires that
     * S11nScope objects are destroyed in the reverse order of
     * creation, and the destructor checks this.
     */
    class S11nScope : public boost::noncopyable {
    public:
	/** @brief Primary constructor.
	 *
	 * @param db Pointer to the CXXR::DB with which
	 *          serialization/deserialization is to be associated.
	 *          If no database is to be used, with the serialized
	 *          data instead being contained within a single XML
	 *          document, the \a db should be a null pointer.
	 *
	 * @param bdg_site_id This parameter is meaningful only if \a
	 *          db is non-null, and is used only during
	 *          serialization, not during deserialization.  When a
	 *          Frame::Binding is being serialized, it contains the
	 *          database's (strictly positive) ID for the binding
	 *          site currently being serialized.  Otherwise it
	 *          should be set to zero.
	 */
	S11nScope(DB* db = 0, unsigned int bdg_site_id = 0)
	    : m_next(s_innermost), m_db(db), m_bdg_site_id(bdg_site_id)
	{
	    s_innermost = this;
	}

	~S11nScope()
	{
	    if (this != s_innermost)
		seqError();
	    s_innermost = m_next;
	}

	/** @brief Binding site ID.
	 *
	 * @return The database's (strictly positive) ID for the
	 * binding currently being serialized.  If no binding is being
	 * serialized, returns 0.
	 */
	unsigned int bdgSiteId() const
	{
	    return m_bdg_site_id;
	}

	/** @brief Database pointer.
	 *
	 * @return Pointer to the CXXR::DB with which the current
	 * serialization/deserialization is associated, or a null
	 * pointer if the current serialization/deserialization is not
	 * associated with a DB.
	 */
	DB* db() const
	{
	    return m_db;
	}

	/** @brief Innermost S11nScope.
	 *
	 * @return pointer to the innermost S11nScope object currently
	 * in existence, or a null pointer if no S11nScope objects
	 * currently exist.
	 */
	static S11nScope* innermost()
	{
	    return s_innermost;
	}
    private:
	static S11nScope* s_innermost;

	S11nScope* m_next;
	DB* m_db;
	unsigned int m_bdg_site_id;

	// Report out-of-sequence destructor call and abort program.
	// (We can't use an exception here because it's called from a
	// destructor.)
	static void seqError();
    };
}  // namespace CXXR

#endif  // S11NSCOPE_HPP
