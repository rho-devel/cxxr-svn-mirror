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

/** @file Symbol.cpp
 *
 * @brief Implementation of class CXXR::Symbol and associated C
 * interface.
 */

#include "CXXR/Symbol.h"

#include <sstream>
#include "localization.h"
#include "boost/regex.hpp"
#include "R_ext/Error.h"
#include "CXXR/Environment.h"
#include "CXXR/Evaluator.h"
#include "CXXR/GCStackRoot.hpp"
#include "CXXR/CachedString.h"

using namespace std;
using namespace CXXR;

namespace CXXR {
    namespace ForceNonInline {
	Rboolean (*DDVALp)(SEXP x) = DDVAL;
	SEXP (*Rf_installp)(const char *name) = Rf_install;
	Rboolean (*isSymbolp)(SEXP s) = Rf_isSymbol;
	SEXP (*PRINTNAMEp)(SEXP x) = PRINTNAME;
    }
}

Symbol::Table* Symbol::s_table = 0;

Symbol* Symbol::s_missing_arg;
SEXP R_MissingArg;

Symbol* Symbol::s_unbound_value;
SEXP R_UnboundValue;

// As of gcc 4.3.2, gcc's std::tr1::regex didn't appear to be working.
// (Discovered 2009-01-16)  So we use boost:

namespace {
    boost::basic_regex<char>* dd_regex;  // "\\.\\.(\\d+)"
}

// ***** Class Symbol itself *****

Symbol::Symbol(const CachedString* the_name)
    : RObject(SYMSXP), m_name(the_name), m_dd_index(0)
{
    if (m_name) {
	if (m_name->size() == 0)
	    Rf_error(_("attempt to use zero-length variable name"));
	if (m_name->size() > maxLength())
	    Rf_error(_("variable names are limited to %d bytes"), maxLength());
    }
    setSelfClone();
    // If this is a ..n symbol, extract the value of n.
    // boost::regex_match (libboost_regex1_36_0-1.36.0-9.5) doesn't
    // seem comfortable with empty strings, hence the size check.
    if (m_name && m_name->size() > 2) {
	string name(m_name->c_str());
	boost::smatch dd_match;
	if (boost::regex_match(name, dd_match, *dd_regex)) {
	    istringstream iss(dd_match[1]);
	    iss >> m_dd_index;
	}
    }
}

void Symbol::cleanup()
{
    // Clearing s_table avoids valgrind 'possibly lost' reports on exit:
    s_table->clear();
}

void Symbol::detachReferents()
{
    m_name.detach();
    RObject::detachReferents();
}

const RObject* Symbol::evaluate(Environment* env) const
{
    if (this == DotsSymbol)
	Rf_error(_("'...' used in an incorrect context"));
    GCStackRoot<const RObject> val;
    if (isDotDotSymbol())
	val = Rf_ddfindVar(const_cast<Symbol*>(this), env);
    else {
	Frame::Binding* bdg = env->findBinding(this).second;
	if (bdg)
	    val = bdg->value();
	else if (this == missingArgument())
	    val = this;  // This reproduces CR behaviour
	else val = unboundValue();
    }
    if (!val)
	return 0;
    if (val.get() == unboundValue())
	Rf_error(_("object '%s' not found"), name()->c_str());
    if (val.get() == missingArgument() && !isDotDotSymbol()) {
	if (name())
	    Rf_error(_("argument \"%s\" is missing, with no default"),
		     name()->c_str());
	else Rf_error(_("argument is missing, with no default"));
    }
    if (val->sexptype() == PROMSXP)
	val = Rf_eval(const_cast<RObject*>(val.get()), env);
    return val;
}

void Symbol::initialize()
{
    static Table table;
    s_table = &table;
    static GCRoot<Symbol> missing_arg(expose(new Symbol));
    s_missing_arg = missing_arg.get();
    R_MissingArg = s_missing_arg;
    static GCRoot<Symbol> unbound_value(expose(new Symbol));
    s_unbound_value = unbound_value.get();
    R_UnboundValue = s_unbound_value;
    static boost::basic_regex<char> dd_rx("\\.\\.(\\d+)");
    dd_regex = &dd_rx;
}

Symbol* Symbol::make(const CachedString* name)
{
    Symbol* ans = CXXR_NEW(Symbol(name));
    s_table->push_back(GCRoot<Symbol>(ans));
    name->m_symbol = ans;
    return ans;
}

const Symbol* Symbol::obtain(const std::string& name)
{
    GCStackRoot<const CachedString> str(CachedString::obtain(name));
    return Symbol::obtain(str);
}

const Symbol* Symbol::obtainDotDotSymbol(unsigned int n)
{
    if (n == 0)
	Rf_error(_("..0 is not a permitted symbol name"));
    ostringstream nameos;
    nameos << ".." << n;
    GCStackRoot<const CachedString> name(CachedString::obtain(nameos.str()));
    return obtain(name);
}

const char* Symbol::typeName() const
{
    return staticTypeName();
}

void Symbol::visitReferents(const_visitor* v) const
{
    const GCNode* name = m_name;
    RObject::visitReferents(v);
    if (name)
	(*v)(name);
}

// Predefined Symbols:
namespace CXXR {
    const Symbol* const Bracket2Symbol = Symbol::obtain("[[");
    const Symbol* const BracketSymbol = Symbol::obtain("[");
    const Symbol* const BraceSymbol = Symbol::obtain("{");
    const Symbol* const TmpvalSymbol = Symbol::obtain("*tmp*");
    const Symbol* const ClassSymbol = Symbol::obtain("class");
    const Symbol* const DeviceSymbol = Symbol::obtain(".Device");
    const Symbol* const DimNamesSymbol = Symbol::obtain("dimnames");
    const Symbol* const DimSymbol = Symbol::obtain("dim");
    const Symbol* const DollarSymbol = Symbol::obtain("$");
    const Symbol* const DotClassSymbol = Symbol::obtain(".Class");
    const Symbol* const DotGenericSymbol = Symbol::obtain(".Generic");
    const Symbol* const DotGenericCallEnvSymbol = Symbol::obtain(".GenericCallEnv");
    const Symbol* const DotGenericDefEnvSymbol = Symbol::obtain(".GenericDefEnv");
    const Symbol* const DotGroupSymbol = Symbol::obtain(".Group");
    const Symbol* const DotMethodSymbol = Symbol::obtain(".Method");
    const Symbol* const DotMethodsSymbol = Symbol::obtain(".Methods");
    const Symbol* const DotdefinedSymbol = Symbol::obtain(".defined");
    const Symbol* const DotsSymbol = Symbol::obtain("...");
    const Symbol* const DottargetSymbol = Symbol::obtain(".target");
    const Symbol* const DropSymbol = Symbol::obtain("drop");
    const Symbol* const ExactSymbol = Symbol::obtain("exact");
    const Symbol* const LastvalueSymbol = Symbol::obtain(".Last.value");
    const Symbol* const LevelsSymbol = Symbol::obtain("levels");
    const Symbol* const ModeSymbol = Symbol::obtain("mode");
    const Symbol* const NameSymbol = Symbol::obtain("name");
    const Symbol* const NamesSymbol = Symbol::obtain("names");
    const Symbol* const NaRmSymbol = Symbol::obtain("na.rm");
    const Symbol* const PackageSymbol = Symbol::obtain("package");
    const Symbol* const PreviousSymbol = Symbol::obtain("previous");
    const Symbol* const QuoteSymbol = Symbol::obtain("quote");
    const Symbol* const RowNamesSymbol = Symbol::obtain("row.names");
    const Symbol* const S3MethodsTableSymbol = Symbol::obtain(".__S3MethodsTable__.");
    const Symbol* const SeedsSymbol = Symbol::obtain(".Random.seed");
    const Symbol* const SourceSymbol = Symbol::obtain("source");
    const Symbol* const TspSymbol = Symbol::obtain("tsp");
    const Symbol* const CommentSymbol = Symbol::obtain("comment");
    const Symbol* const DotEnvSymbol = Symbol::obtain(".Environment");
    const Symbol* const RecursiveSymbol = Symbol::obtain("recursive");
    const Symbol* const UseNamesSymbol = Symbol::obtain("use.names");
    const Symbol* const SrcfileSymbol = Symbol::obtain("srcfile");
    const Symbol* const SrcrefSymbol = Symbol::obtain("srcref");
    const Symbol* const WholeSrcrefSymbol = Symbol::obtain("wholeSrcref");
}

// ***** C interface *****

SEXP R_Bracket2Symbol = const_cast<Symbol*>(CXXR::Bracket2Symbol);
SEXP R_BracketSymbol = const_cast<Symbol*>(CXXR::BracketSymbol);
SEXP R_BraceSymbol = const_cast<Symbol*>(CXXR::BraceSymbol);
SEXP R_ClassSymbol = const_cast<Symbol*>(CXXR::ClassSymbol);
SEXP R_DeviceSymbol = const_cast<Symbol*>(CXXR::DeviceSymbol);
SEXP R_DimNamesSymbol = const_cast<Symbol*>(CXXR::DimNamesSymbol);
SEXP R_DimSymbol = const_cast<Symbol*>(CXXR::DimSymbol);
SEXP R_DollarSymbol = const_cast<Symbol*>(CXXR::DollarSymbol);
SEXP R_DotsSymbol = const_cast<Symbol*>(CXXR::DotsSymbol);
SEXP R_DropSymbol = const_cast<Symbol*>(CXXR::DropSymbol);
SEXP R_LastvalueSymbol = const_cast<Symbol*>(CXXR::LastvalueSymbol);
SEXP R_LevelsSymbol = const_cast<Symbol*>(CXXR::LevelsSymbol);
SEXP R_ModeSymbol = const_cast<Symbol*>(CXXR::ModeSymbol);
SEXP R_NameSymbol = const_cast<Symbol*>(CXXR::NameSymbol);
SEXP R_NamesSymbol = const_cast<Symbol*>(CXXR::NamesSymbol);
SEXP R_NaRmSymbol = const_cast<Symbol*>(CXXR::NaRmSymbol);
SEXP R_PackageSymbol = const_cast<Symbol*>(CXXR::PackageSymbol);
SEXP R_QuoteSymbol = const_cast<Symbol*>(CXXR::QuoteSymbol);
SEXP R_RowNamesSymbol = const_cast<Symbol*>(CXXR::RowNamesSymbol);
SEXP R_SeedsSymbol = const_cast<Symbol*>(CXXR::SeedsSymbol);
SEXP R_SourceSymbol = const_cast<Symbol*>(CXXR::SourceSymbol);
SEXP R_TspSymbol = const_cast<Symbol*>(CXXR::TspSymbol);

SEXP R_CommentSymbol = const_cast<Symbol*>(CXXR::CommentSymbol);
SEXP R_DotEnvSymbol = const_cast<Symbol*>(CXXR::DotEnvSymbol);
SEXP R_ExactSymbol = const_cast<Symbol*>(CXXR::ExactSymbol);
SEXP R_RecursiveSymbol = const_cast<Symbol*>(CXXR::RecursiveSymbol);
SEXP R_SrcfileSymbol = const_cast<Symbol*>(CXXR::SrcfileSymbol);
SEXP R_SrcrefSymbol = const_cast<Symbol*>(CXXR::SrcrefSymbol);
SEXP R_WholeSrcrefSymbol = const_cast<Symbol*>(CXXR::WholeSrcrefSymbol);
SEXP R_TmpvalSymbol = const_cast<Symbol*>(CXXR::TmpvalSymbol);
SEXP R_UseNamesSymbol = const_cast<Symbol*>(CXXR::UseNamesSymbol);

// Rf_install() is currently defined in main.cpp

