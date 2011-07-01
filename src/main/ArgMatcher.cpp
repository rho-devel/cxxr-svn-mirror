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

/** @file ArgMatcher.cpp
 *
 * Implementation of class ArgMatcher.
 */

#include "CXXR/ArgMatcher.hpp"

#include "CXXR/ArgList.hpp"
#include "CXXR/DottedArgs.hpp"
#include "CXXR/Environment.h"
#include "CXXR/GCStackRoot.hpp"
#include "CXXR/PairList.h"
#include "CXXR/Promise.h"
#include "CXXR/Symbol.h"
#include "CXXR/errors.h"

using namespace std;
using namespace CXXR;

bool ArgMatcher::s_warn_on_partial_match = false;

void ArgMatcher::build()
{
    // We deliberately iterate using a non-const pointer, so that lazy
    // copies are forced, and consequently objects pointed to by
    // m_formal_index will stay in place.
    for (PairList* f = m_formals.get(); f; f = f->tail()) {
	const Symbol* sym = dynamic_cast<const Symbol*>(f->tag());
	if (!sym)
	    Rf_error(_("invalid formal arguments for 'function'"));
	if (sym == DotsSymbol) {
	    if (m_has_dots)
		Rf_error(_("formals list contains more than one '...'"));
	    if (f->car() != Symbol::missingArgument())
		Rf_error(_("'...' formal cannot have a default value"));
	    m_has_dots = true;
	} else {
	    FormalData fdata = {sym, m_has_dots, f->car()};
	    pair<FormalMap::const_iterator, bool> pr =
		m_formal_index.insert(make_pair(sym->name(),
						m_formal_data.size()));
	    if (!pr.second)
		Rf_error(_("duplicated name in formals list"));
	    m_formal_data.push_back(fdata);
	}
    }
}

void ArgMatcher::detachReferents()
{
    m_formals.detach();
    m_formals2.detach();
    m_formal_data.clear();
    m_formal_index.clear();
}

void ArgMatcher::handleDots(Frame* frame, SuppliedList* supplied_list)
{
    Frame::Binding* bdg = frame->obtainBinding(DotsSymbol);
    if (!supplied_list->empty()) {
	SuppliedList::iterator first = supplied_list->begin();
	DottedArgs* dotted_args
	    = expose(new DottedArgs((*first).value, 0, (*first).tag));
	bdg->setValue(dotted_args, Frame::Binding::EXPLICIT);
	supplied_list->erase(first);
	GCStackRoot<PairList> tail;
	for (SuppliedList::const_reverse_iterator rit = supplied_list->rbegin();
	     rit != supplied_list->rend(); ++rit)
	    tail = PairList::cons((*rit).value, tail, (*rit).tag);
	dotted_args->setTail(tail);
	supplied_list->clear();
    }
}
	     
bool ArgMatcher::isPrefix(const CachedString* shorter,
			  const CachedString* longer)
{
    const string& shortstr = shorter->stdstring();
    const string& longstr = longer->stdstring();
    return longstr.compare(0, shortstr.size(), shortstr) == 0;
}

ArgMatcher* ArgMatcher::make(const Symbol* fml1, const Symbol* fml2,
			     const Symbol* fml3, const Symbol* fml4,
			     const Symbol* fml5, const Symbol* fml6)
{
    GCStackRoot<PairList> formals;
    if (fml6)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml6);
    if (fml5)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml5);
    if (fml4)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml4);
    if (fml3)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml3);
    if (fml2)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml2);
    if (fml1)
	formals = PairList::cons(Symbol::missingArgument(), formals, fml1);
    return expose(new ArgMatcher(formals));
}

void ArgMatcher::makeBinding(Environment* target_env, const FormalData& fdata,
			     const RObject* supplied_value)
{
    const RObject* value = supplied_value;
    Frame::Binding::Origin origin = Frame::Binding::EXPLICIT;
    if (value == Symbol::missingArgument()
	&& fdata.value != Symbol::missingArgument()) {
	origin = Frame::Binding::DEFAULTED;
	value = expose(new Promise(fdata.value, target_env));
    }
    Frame::Binding* bdg = target_env->frame()->obtainBinding(fdata.symbol);
    // Don't trump a previous binding with Symbol::missingArgument() :
    if (value != Symbol::missingArgument())
	bdg->setValue(value, origin);
}
    
void ArgMatcher::match(Environment* target_env, const ArgList* supplied) const
{
    Frame* frame = target_env->frame();
    vector<MatchStatus, Allocator<MatchStatus> >
	formals_status(m_formal_data.size(), UNMATCHED);
    SuppliedList supplied_list;
    // Exact matches by tag:
    {
	unsigned int sindex = 0;
	for (const PairList* s = supplied->list(); s; s = s->tail()) {
	    ++sindex;
	    const Symbol* tag = static_cast<const Symbol*>(s->tag());
	    const CachedString* name = (tag ? tag->name() : 0);
	    const RObject* value = s->car();
	    FormalMap::const_iterator fmit 
		= (name ? m_formal_index.lower_bound(name)
		   : m_formal_index.end());
	    if (fmit != m_formal_index.end() && (*fmit).first == name) {
		// Exact tag match:
		unsigned int findex = (*fmit).second;
		const FormalData& fdata = m_formal_data[findex];
		formals_status[findex] = EXACT_TAG;
		makeBinding(target_env, fdata, value);
	    } else {
		// No exact tag match, so place supplied arg on list:
		SuppliedData supplied_data
		    = {tag, value, fmit, sindex};
		supplied_list.push_back(supplied_data);
	    }
	}
    }
    // Partial matches by tag:
    {
	SuppliedList::iterator slit = supplied_list.begin();
	while (slit != supplied_list.end()) {
	    SuppliedList::iterator next = slit;
	    ++next;
	    const SuppliedData& supplied_data = *slit;
	    const CachedString* supplied_name
		= (supplied_data.tag ? supplied_data.tag->name() : 0);
	    FormalMap::const_iterator fmit = supplied_data.fm_iter;
	    // Within m_formal_index, skip formals formals following
	    // '...' and formals with exact matches:
	    while (fmit != m_formal_index.end()
		   && (m_formal_data[(*fmit).second].follows_dots
		       || formals_status[(*fmit).second] == EXACT_TAG))
		++fmit;
	    if (fmit != m_formal_index.end()
		&& isPrefix(supplied_name, (*fmit).first)) {
		// This is a potential partial match.  Remember the formal:
		unsigned int findex = (*fmit).second;
		// Has this formal already been partially matched? :
		if (formals_status[(*fmit).second] == PARTIAL_TAG)
		    Rf_error(_("formal argument '%s' matched by "
			       "multiple actual arguments"),
			     (*fmit).first->c_str());
		// Does supplied arg partially match anything else? :
		do {
		    ++fmit;
		} while (fmit != m_formal_index.end()
			 && formals_status[(*fmit).second] == EXACT_TAG);
		if (fmit != m_formal_index.end()
		    && isPrefix(supplied_name, (*fmit).first))
		    Rf_error(_("argument %d matches multiple formal arguments"),
			     supplied_data.index);
		// Partial match is OK:
		if (s_warn_on_partial_match)
		    Rf_warning(_("partial argument match of '%s' to '%s'"),
			       supplied_name->c_str(), (*fmit).first->c_str());
		const FormalData& fdata = m_formal_data[findex];
		formals_status[findex] = PARTIAL_TAG;
		makeBinding(target_env, fdata, supplied_data.value);
		supplied_list.erase(slit);
	    }
	    slit = next;
	}
    }
    // Positional matching and default values:
    {
	const size_t numformals = m_formal_data.size();
	SuppliedList::iterator slit = supplied_list.begin();
	for (unsigned int findex = 0; findex < numformals; ++findex) {
	    if (formals_status[findex] == UNMATCHED) {
		const FormalData& fdata = m_formal_data[findex];
		const RObject* value = Symbol::missingArgument();
		// Skip supplied arguments with tags:
		while (slit != supplied_list.end() && (*slit).tag)
		    ++slit;
		if (slit != supplied_list.end()
		    && !fdata.follows_dots) {
		    // Handle positional match:
		    const SuppliedData& supplied_data = *slit;
		    value = supplied_data.value;
		    formals_status[findex] = POSITIONAL;
		    supplied_list.erase(slit++);
		}
		makeBinding(target_env, fdata, value);
	    }
	}
    }
    // Any remaining supplied args are either rolled into ... or
    // there's an error:
    if (m_has_dots)
	handleDots(frame, &supplied_list);
    else if (!supplied_list.empty())
	unusedArgsError(supplied_list);
}

void ArgMatcher::propagateFormalBindings(const Environment* fromenv,
					 Environment* toenv) const
{
    const Frame* fromf = fromenv->frame();
    for (FormalVector::const_iterator it = m_formal_data.begin();
	 it != m_formal_data.end(); ++it) {
	const FormalData& fdata = *it;
	const Symbol* symbol = fdata.symbol;
	const Frame::Binding* frombdg = fromf->binding(symbol);
	if (!frombdg)
	    Rf_error(_("could not find symbol \"%s\" "
		       "in environment of the generic function"),
		     symbol->name()->c_str());
	const RObject* val = frombdg->value();
	// Discard generic's defaults:
	if (frombdg->origin() != Frame::Binding::EXPLICIT)
	    val = Symbol::missingArgument();
	makeBinding(toenv, fdata, val);
    }
    // m_formal_data excludes '...', so:
    if (m_has_dots) {
	const Frame::Binding* frombdg = fromf->binding(DotsSymbol);
	Frame::Binding* tobdg = toenv->frame()->obtainBinding(DotsSymbol);
	tobdg->setValue(frombdg->value(), frombdg->origin());
    }
}
	    
void ArgMatcher::stripFormals(Frame* input_frame) const
{
    const PairList* fcell = m_formals;
    while (fcell) {
	input_frame->erase(static_cast<const Symbol*>(fcell->tag()));
	fcell = fcell->tail();
    }
}

// Implementation of ArgMatcher::unusedArgsError() is in match.cpp

void ArgMatcher::visitReferents(const_visitor* v) const
{
    if (m_formals)
	(*v)(m_formals);
    if (m_formals2)
	(*v)(m_formals2);
}
