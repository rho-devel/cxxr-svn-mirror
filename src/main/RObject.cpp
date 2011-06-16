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
 *  Copyright (C) 1999-2007   The R Development Core Team.
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

/** @file RObject.cpp
 *
 * Class RObject and associated C interface functions.
 */

#include "CXXR/RObject.h"

#include <cstdlib>
#include <iostream>
#include "localization.h"
#include "R_ext/Error.h"
#include "CXXR/GCStackRoot.hpp"
#include "CXXR/PairList.h"
#include "CXXR/Symbol.h"

using namespace std;
using namespace CXXR;

// Force the creation of non-inline embodiments of functions callable
// from C:
namespace CXXR {
    namespace ForceNonInline {
	void (*DUPLICATE_ATTRIBptr)(SEXP, SEXP) = DUPLICATE_ATTRIB;
	Rboolean (*isNullptr)(SEXP s) = Rf_isNull;
	Rboolean (*isObjectptr)(SEXP s) = Rf_isObject;
	Rboolean (*IS_S4_OBJECTptr)(SEXP x) = IS_S4_OBJECT;
	int (*NAMEDptr)(SEXP x) = NAMED;
	Rboolean (*OBJECTptr)(SEXP e) = OBJECT;
	void (*SET_S4_OBJECTptr)(SEXP x) = SET_S4_OBJECT;
	SEXPTYPE (*TYPEOFptr)(SEXP e) = TYPEOF;
	void (*UNSET_S4_OBJECTptr)(SEXP x) = UNSET_S4_OBJECT;
    }
}

namespace {
    // Used in {,un}packGPBits():
    const unsigned int S4_OBJECT_MASK = 1<<4;
}

const unsigned char RObject::s_sexptype_mask;
const unsigned char RObject::s_S4_mask;
const unsigned char RObject::s_class_mask;

RObject::RObject(const RObject& pattern)
    : m_type(pattern.m_type), m_owners(0), m_missing(pattern.m_missing),
      m_argused(pattern.m_argused), m_active_binding(pattern.m_active_binding),
      m_binding_locked(pattern.m_binding_locked), m_attrib(pattern.m_attrib)
{}

const PairList* RObject::attributes() const
{
    return m_attrib;
}

void RObject::clearAttributes()
{
    if (m_attrib) {
	m_attrib = 0;
	m_type &= ~s_class_mask;
    }
}

void RObject::copyAttributes(const RObject* source, bool copyS4)
{
    const PairList* srcatts = source->attributes();
    GCStackRoot<const PairList> attribs(srcatts ? srcatts->clone() : 0);
    setAttributes(attribs);
    if (copyS4)
	setS4Object(source->isS4Object());
}

RObject* RObject::evaluate(Environment* env)
{
    if (NAMED(this) != 2)
	SET_NAMED(this, 2);
    return this;
}

RObject* RObject::getAttribute(const Symbol* name) const
{
    for (PairList* node = m_attrib; node; node = node->tail())
	if (node->tag() == name)
	    return node->car();
    return 0;
}

unsigned int RObject::packGPBits() const
{
    unsigned int ans = 0;
    if (isS4Object())
	ans |= S4_OBJECT_MASK;
    return ans;
}

// This follows CR in adding new attributes at the end of the list,
// though it would be easier to add them at the beginning.
void RObject::setAttribute(const Symbol* name, RObject* value)
{
    if (!name)
	Rf_error(_("attributes must be named"));
    // Update 'has class' bit if necessary:
    if (name == R_ClassSymbol) {
	if (value == 0)
	    m_type &= ~s_class_mask;
	else m_type |= s_class_mask;
    }
    // Find attribute:
    PairList* prev = 0;
    PairList* node = m_attrib;
    while (node && node->tag() != name) {
	prev = node;
	node = node->tail();
    }
    if (node) {  // Attribute already present
	// Update existing attribute:
	if (value)
	    node->setCar(value);
	// Delete existing attribute:
	else if (prev)
	    prev->setTail(node->tail());
	else m_attrib = node->tail();
    } else if (value) {  
	// Create new node:
	PairList* newnode = PairList::cons(value, 0, name);
	if (prev)
	    prev->setTail(newnode);
	else { // No preexisting attributes at all:
	    m_attrib = newnode;
	}
    }
}

// This has complexity O(n^2) where n is the number of attributes, but
// we assume n is very small.    
void RObject::setAttributes(const PairList* new_attributes)
{
    clearAttributes();
    while (new_attributes) {
	const Symbol* name
	    = SEXP_downcast<const Symbol*>(new_attributes->tag());
	setAttribute(name, new_attributes->car());
	new_attributes = new_attributes->tail();
    }
}

void RObject::setS4Object(bool on)
{
    // Check suppressed (temporarily I hope) during upgrade to R 2.8.1:
    // if (!on && sexptype() == S4SXP)
    //      Rf_error("S4 object (S4SXP) cannot cease to be an S4 object.");
    if (on)
	m_type |= s_S4_mask;
    else m_type &= ~s_S4_mask;
}

const char* RObject::typeName() const
{
    return Rf_type2char(sexptype());
}

void RObject::unpackGPBits(unsigned int gpbits)
{
    // Be careful with precedence!
    setS4Object((gpbits & S4_OBJECT_MASK) != 0);
}

void RObject::visitReferents(const_visitor* v) const
{
    if (m_attrib)
	(*v)(m_attrib);
}

// ***** C interface *****

SEXP ATTRIB(SEXP x)
{
    GCNode::GCInhibitor inhibitor;
    return x ? const_cast<PairList*>(x->attributes()) : 0;
}

void DUPLICATE_ATTRIB(SEXP to, SEXP from)
{
    if (from) 
	to->copyAttributes(from, true);
    else {
	to->clearAttributes();
	to->setS4Object(false);
    }
}

void SET_ATTRIB(SEXP x, SEXP v)
{
    GCStackRoot<PairList> pl(SEXP_downcast<PairList*>(v));
    GCNode::GCInhibitor inhibitor;
    x->setAttributes(pl);
}
