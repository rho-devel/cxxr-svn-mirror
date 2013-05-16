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

/** @file DB.hpp
 *
 * @brief Class CXXR::DB.
 */

#ifndef CXXR_DB_HPP
#define CXXR_DB_HPP 1

class sqlite3;

namespace CXXR {
    /** @brief Database for persistent data.
     *
     * CXXR, unlike CR, stores its persistent data in an SQL database.
     * This singleton class provides the interface to that database.
     *
     * The class currently assumes the use of an SQLite database.
     */
    class DB {
    public:
	/** @brief Constructor.
	 *
	 * At most one DB object can exist at any time.
	 *
	 * @param filename Path to the file containing the database.
	 *          If the file does not exist, it will be created and
	 *          a new database constructed within it.
	 */
	DB(const char* filename);

	~DB();

	/** @brief Register a new persistent Frame.
	 *
	 * @return The (strictly positive) id number to be used to
	 * identify the Frame.
	 */
	unsigned int registerFrame();

	/** @brief Pointer to the current database.
	 *
	 * @return a null pointer if no DB object currently exists,
	 * otherwise a pointer to the unique DB object.
	 */
	static DB* theDB()
	{
	    return s_db;
	}
    private:
	static DB* s_db;

	sqlite3* m_db;

	// Check database structure.  If the database does not already
	// contain any CXXR tables (e.g. because the database is newly
	// created), create them by calling initialize().
	void check();

	void db_error(const char* message_prefix);

	void initialize();
    };
}

#endif  // CXXR_DB_HPP
