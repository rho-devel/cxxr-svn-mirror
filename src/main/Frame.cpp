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

/** @file Frame.cpp
 *
 * @brief Implementation of class CXXR:Frame and CXXR::Frame::Binding.
 */

#include "CXXR/Frame.hpp"

#include "localization.h"
#include "R_ext/Error.h"
#include "CXXR/Evaluator.h"
#include "CXXR/FunctionBase.h"
#include "CXXR/GCStackRoot.hpp"
#include "CXXR/Promise.h"

using namespace std;
using namespace CXXR;

Frame::monitor Frame::s_read_monitor = 0;
Frame::monitor Frame::s_write_monitor = 0;

// ***** Class Frame::Binding *****

PairList* Frame::Binding::asPairList(PairList* tail) const
{
    PairList* ans = new PairList(m_value, tail, symbol());
    SET_MISSING(ans, origin());
    if (isActive()) SET_ACTIVE_BINDING_BIT(ans);
    if (isLocked()) LOCK_BINDING(ans);
    return expose(ans);
}

// Frame::Binding::assign() is defined in envir.cpp (for the time being).
	
pair<const RObject*, bool>
Frame::Binding::forcedValue() const
{
    bool promise_forced = false;
    const RObject* val = m_value;
    if (val && val->sexptype() == PROMSXP) {
	const Promise* prom = static_cast<const Promise*>(val);
	if (prom->environment()) {
	    GCStackRoot<const Promise> promrt(prom);
	    frame()->monitorRead(*this);
	    val = evaluate(val, 0);
	    promise_forced = true;
	}
	val = prom->value();
    }
    return make_pair(val, promise_forced);
}

void Frame::Binding::fromPairList(PairList* pl)
{
    const RObject* tag = pl->tag();
    if (tag && tag != m_symbol)
	Rf_error(_("internal error in %s"),
		 "Frame::Binding::fromPairList()");
    Origin pl_origin = Origin(pl->m_missing);
    if (pl->m_active_binding)
	setFunction(SEXP_downcast<FunctionBase*>(pl->car()),
		    pl_origin);
    else setValue(pl->car(), pl_origin);
    setLocking(pl->m_binding_locked);
}
    
void Frame::Binding::initialize(Frame* frame, const Symbol* sym)
{
    if (m_frame)
	Rf_error(_("internal error: binding already initialized"));
    if (!frame || !sym)
	Rf_error(_("internal error in %s"),
		 "Frame::Binding::initialize()");
    m_frame = frame;
    m_symbol = sym;
}

void Frame::Binding::setFunction(const FunctionBase* function, Origin origin)
{
    // See if binding already has a non-default value:
    if (m_value != Symbol::missingArgument()) {
	if (!isActive())
	    Rf_error(_("symbol already has a regular binding"));
	if (isLocked())
	    Rf_error(_("cannot change active binding if binding is locked"));
    }
    m_value = function;
    m_active = true;
    m_frame->monitorWrite(*this);
}

void Frame::Binding::setValue(const RObject* new_value, Origin origin)
{
    if (isLocked())
	Rf_error(_("cannot change value of locked binding for '%s'"),
		 symbol()->name()->c_str());
    if (isActive())
	Rf_error(_("internal error: use %s for active bindings"),
		 "setFunction()");
    m_value = new_value;
    m_origin = origin;
    m_frame->monitorWrite(*this);
}

// Frame::Binding::value() is defined in envir.cpp (for the time being).
	
void Frame::Binding::visitReferents(const_visitor* v) const
{
    // We assume the visitor has just come from m_frame, so we don't
    // visit that.
    if (m_value)
	(*v)(m_value);
}

void Frame::flush(const Symbol* sym)
{
    Environment::flushFromCache(sym);
}

namespace CXXR {
    void frameReadPairList(Frame* frame, PairList* bindings)
    {
	for (PairList* pl = bindings; pl != 0; pl = pl->tail()) {
	    const RObject* tag = pl->tag();
	    const Symbol* symbol = dynamic_cast<const Symbol*>(tag);
	    if (!symbol) Rf_error(_("list used to set frame bindings"
				    " must have symbols as tags throughout"));
	    Frame::Binding* bdg = frame->obtainBinding(symbol);
	    bdg->fromPairList(pl);
	}
    }

    bool isMissingArgument(const Symbol* sym, Frame* frame)
    {
	const RObject* rawval;
	if (sym->isDotDotSymbol()) {
	    unsigned int ddv = sym->dotDotIndex();
	    Frame::Binding* bdg = frame->binding(CXXR::DotsSymbol);
	    if (!bdg)
		return false;  // This is what CR does.  Is it really right?
	    const ConsCell* cc = SEXP_downcast<const ConsCell*>(bdg->rawValue());
	    while (cc && ddv > 1) {
		cc = cc->tail();
		--ddv;
	    }
	    if (!cc)
		return true;
	    rawval = cc->car();
	} else {
	    // Not a ..n symbol:
	    if (sym == Symbol::missingArgument())
		return true;
	    Frame::Binding* bdg = frame->binding(sym);
	    if (!bdg)
		return false;
	    rawval = bdg->rawValue();
	    if (bdg->origin() == Frame::Binding::MISSING
		|| rawval == Symbol::missingArgument())
		return  true;
	    if (bdg->isActive())
		return false;
	}
	if (rawval && rawval->sexptype() == PROMSXP) {
	    const Promise* prom = static_cast<const Promise*>(rawval);
	    return prom->isMissingSymbol();
	}
	return false;
    }
}
