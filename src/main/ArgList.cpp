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

/** @file ArgList.cpp
 *
 * Implementation of class ArgList.
 */

#include "CXXR/ArgList.hpp"

#include <list>
#include "CXXR/DottedArgs.hpp"
#include "CXXR/Environment.h"
#include "CXXR/Evaluator.h"
#include "CXXR/Promise.h"
#include "CXXR/errors.h"

using namespace std;
using namespace CXXR;

// Implementation of ArgList::coerceTag() is in coerce.cpp

void ArgList::evaluate(Environment* env, bool allow_missing)
{
    if (m_status == EVALUATED)
	Rf_error("Internal error: ArgList already evaluated");
    if (m_first_arg_env && env != m_first_arg_env)
	Rf_error("Internal error: first arg of ArgList"
		 " previously evaluated in different environment");
    GCStackRoot<const PairList> oldargs(list());
    m_list->setTail(0);
    PairList* lastout = m_list;
    unsigned int arg_number = 1;
    for (const PairList* inp = oldargs; inp; inp = inp->tail()) {
	const RObject* incar = inp->car();
	if (incar == DotsSymbol) {
	    Frame::Binding* bdg = env->findBinding(CXXR::DotsSymbol).second;
	    if (!bdg)
		Rf_error(_("'...' used but not bound"));
	    const RObject* h = bdg->value();
	    if (!h || h->sexptype() == DOTSXP) {
		const ConsCell* dotlist = static_cast<const DottedArgs*>(h);
		while (dotlist) {
		    const RObject* dotcar = dotlist->car();
		    const RObject* outcar = Symbol::missingArgument();
		    if (m_first_arg_env) {
			outcar = m_first_arg;
			m_first_arg = 0;
			m_first_arg_env = 0;
		    } else if (dotcar != Symbol::missingArgument())
			outcar = Evaluator::evaluate(dotcar, env);
		    PairList* cell = PairList::cons(outcar, 0, dotlist->tag());
		    lastout->setTail(cell);
		    lastout = lastout->tail();
		    dotlist = dotlist->tail();
		}
	    } else if (h != Symbol::missingArgument())
		Rf_error(_("'...' used in an incorrect context"));
	} else {
	    const RObject* tag = inp->tag();
	    PairList* cell = 0;
	    if (m_first_arg_env) {
		cell = PairList::cons(m_first_arg, 0, tag);
		m_first_arg = 0;
		m_first_arg_env = 0;
	    } else if (incar && incar->sexptype() == SYMSXP) {
		const Symbol* sym = static_cast<const Symbol*>(incar);
		if (sym == Symbol::missingArgument()) {
		    if (allow_missing)
			cell = PairList::cons(Symbol::missingArgument(), 0, tag);
		    else Rf_error(_("argument %d is empty"), arg_number);
		} else if (isMissingArgument(sym, env->frame())) {
		    if (allow_missing)
			cell = PairList::cons(Symbol::missingArgument(), 0, tag);
		    else Rf_error(_("'%s' is missing"), sym->name()->c_str());
		}
	    }
	    if (!cell) {
		const RObject* outcar = Evaluator::evaluate(incar, env);
		cell = PairList::cons(outcar, 0, inp->tag());
	    }
	    lastout->setTail(cell);
	    lastout = lastout->tail();
	}
	++arg_number;
    }
    m_status = EVALUATED;
}

void ArgList::merge(const ConsCell* extraargs)
{
    if (m_status != PROMISED)
	Rf_error("Internal error: ArgList::merge() requires PROMISED ArgList");
    // Convert extraargs into a doubly linked list:
    typedef std::list<pair<const RObject*, const RObject*> > Xargs;
    Xargs xargs;
    for (const ConsCell* cc = extraargs; cc; cc = cc->tail())
	xargs.push_back(make_pair(cc->tag(), cc->car()));
    // Duplicate the original list if necessary:
    if (list() == m_orig_list)
	m_list->setTail(m_orig_list->clone());
    // Apply overriding arg values supplied in extraargs:
    PairList* last = m_list;
    for (PairList* pl = m_list->tail(); pl; pl = pl->tail()) {
	last = pl;
	const RObject* tag = pl->tag();
	if (tag) {
	    Xargs::iterator it = xargs.begin();
	    while (it != xargs.end() && (*it).first != tag)
		++it;
	    if (it != xargs.end()) {
		pl->setCar((*it).second);
		xargs.erase(it);
	    }
	}
    }
    // Append remaining extraargs:
    for (Xargs::const_iterator it = xargs.begin(); it != xargs.end(); ++it) {
	last->setTail(PairList::cons((*it).second, 0, (*it).first));
	last = last->tail();
    }
}

pair<bool, const RObject*> ArgList::firstArg(Environment* env)
{
    const PairList* elt = list();
    if (!elt)
	return pair<bool, RObject*>(false, 0);
    if (m_status == EVALUATED)
	return make_pair(true, elt->car());
    while (elt) {
	const RObject* arg1 = elt->car();
	if (!arg1)
	    return pair<bool, RObject*>(true, 0);
	if (arg1 != DotsSymbol) {
	    m_first_arg = arg1->evaluate(env);
	    m_first_arg_env = env;
	    return make_pair(true, m_first_arg);
	}
	// If we get here it must be DotSymbol.
	Frame::Binding* bdg = env->findBinding(DotsSymbol).second;
	if (bdg && bdg->origin() != Frame::Binding::MISSING) {
	    const RObject* val = bdg->value();
	    if (val) {
		if (val->sexptype() != DOTSXP)
		    Rf_error(_("'...' used in an incorrect context"));
		const RObject* dots1
		    = static_cast<const DottedArgs*>(val)->car();
		if (dots1->sexptype() != PROMSXP)
		    Rf_error(_("value in '...' is not a promise"));
		m_first_arg = dots1->evaluate(env);
		m_first_arg_env = env;
		return make_pair(true, m_first_arg);
	    }
	}
	elt = elt->tail();  // elt was unbound or missing DotsSymbol
    }
    return pair<bool, RObject*>(false, 0);
}

void ArgList::stripTags()
{
    if (list() == m_orig_list) {
	// Mustn't modify the list supplied to the constructor:
	GCStackRoot<PairList> oldargs(m_list->tail());
	m_list->setTail(0);
	PairList* lastout = m_list;
	for (const PairList* inp = oldargs; inp; inp = inp->tail()) {
	    lastout->setTail(PairList::cons(inp->car(), 0));
	    lastout = lastout->tail();
	}
    } else {
	for (PairList* p = m_list->tail(); p; p = p->tail())
	    p->setTag(0);
    }
}
	    
void ArgList::wrapInPromises(Environment* env)
{
    if (m_status == PROMISED)
	Rf_error("Internal error:"
		 " ArgList already wrapped in Promises");
    if (m_status == EVALUATED)
	env = 0;
    else if (m_first_arg_env && env != m_first_arg_env)
	Rf_error("Internal error: first arg of ArgList"
		 " previously evaluated in different environment");
    GCStackRoot<PairList> oldargs(m_list->tail());
    m_list->setTail(0);
    PairList* lastout = m_list;
    for (const PairList* inp = oldargs; inp; inp = inp->tail()) {
	const RObject* rawvalue = inp->car();
	if (rawvalue == DotsSymbol) {
	    pair<Environment*, Frame::Binding*> pr
		= env->findBinding(DotsSymbol);
	    if (pr.first) {
		const RObject* dval = pr.second->value();
		if (!dval || dval->sexptype() == DOTSXP) {
		    const ConsCell* dotlist = static_cast<const ConsCell*>(dval);
		    while (dotlist) {
			Promise* prom;
			if (!m_first_arg_env)
			    prom = new Promise(dotlist->car(), env);
			else {
			    prom = new Promise(m_first_arg, 0);
			    m_first_arg = 0;
			    m_first_arg_env = 0;
			}
			prom->expose();
			const Symbol* tag = tag2Symbol(dotlist->tag());
			lastout->setTail(PairList::cons(prom, 0, tag));
			lastout = lastout->tail();
			dotlist = dotlist->tail();
		    }
		} else if (dval != Symbol::missingArgument())
		    Rf_error(_("'...' used in an incorrect context"));
	    }
	} else {
	    const Symbol* tag = tag2Symbol(inp->tag());
	    const RObject* value = Symbol::missingArgument();
	    if (m_first_arg_env) {
		value = CXXR_NEW(Promise(m_first_arg, 0));
		m_first_arg = 0;
		m_first_arg_env = 0;
	    } else if (rawvalue != Symbol::missingArgument())
		value = CXXR_NEW(Promise(rawvalue, env));
	    lastout->setTail(PairList::cons(value, 0, tag));
	    lastout = lastout->tail();
	}
    }
    m_status = PROMISED;
}
