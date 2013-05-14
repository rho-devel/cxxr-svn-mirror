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

/** @file DB.cpp
 *
 * Implementation of class CXXR::DB.
 */

#include "CXXR/DB.hpp"

#include <cstdlib>
#include <iostream>
#include <sqlite3.h>
#include "CXXR/errors.h"

using namespace CXXR;

DB* DB::s_db = 0;

DB::DB(const char* filename)
    : m_db(0)
{
    if (s_db) {
	std::cerr << "Internal error: at most one CXXR::DB can exist.\n";
	std::abort();
    }
    sqlite3_initialize();
    int status = sqlite3_open_v2(filename, &m_db,
				 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    if (status != SQLITE_OK)
	db_error("cannot open database");
    check();
    s_db = this;
}

DB::~DB()
{
    s_db = 0;
    if (sqlite3_close(m_db) != SQLITE_OK) {
	std::cerr << "Internal error: cannot close database.\n";
	std::abort();
    }
}

void DB::check()
{
    const char* sql =
	"SELECT count(*) FROM sqlite_master"
	" WHERE type == \"table\" AND tbl_name LIKE \"cxxr%\";";
    sqlite3_stmt* stmt;
    if (SQLITE_OK != sqlite3_prepare_v2(m_db, sql, -1, &stmt, 0))
	db_error("error checking database");
    if (SQLITE_ROW != sqlite3_step(stmt))
	db_error("error 2 checking database");
    if (SQLITE_NULL == sqlite3_column_type(stmt, 0))
	db_error("error 3 checking database");
    int cxxr_tbl_count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    // std::cout << "cxxr_tbl_count: " << cxxr_tbl_count << std::endl;
    if (cxxr_tbl_count == 0)
	initialize();
    else if (cxxr_tbl_count != 4)
	Rf_error(_("corrupt database"));
}
    
void DB::db_error(const char* message_prefix)
{
    std::string msg = _(message_prefix);
    msg += ": ";
    msg += sqlite3_errmsg(m_db);
    Rf_error(msg.c_str());
}

void DB::initialize()
{
    const char* sql =
	"CREATE TABLE cxxr_frames (\n"
        "  frame_id integer primary key\n"
        ");\n"
	"CREATE TABLE cxxr_binding_sites (\n"
        "  bdgsite_id integer primary key,\n"
	"  frame_id integer not null references cxxr_frames,\n"
	"  symbol text not null\n"
	");\n"
	"CREATE TABLE cxxr_live_bindings (\n"
	"  bdg_id INTEGER PRIMARY KEY,\n"
	"  bdgsite_id INTEGER UNIQUE NOT NULL REFERENCES cxxr_binding_sites,\n"
	"  value TEXT NOT NULL\n"
	");\n"
	"CREATE TABLE cxxr_frame_references (\n"
	"  bdg_id integer not null references cxxr_bindings,\n"
	"  frame_id integer not null references cxxr_frames,\n"
	"  unique( bdg_id, frame_id)\n"
	");\n";
    if (sqlite3_exec(m_db, sql, 0, 0, 0) != SQLITE_OK)
	db_error("cannot initialize database");
}
