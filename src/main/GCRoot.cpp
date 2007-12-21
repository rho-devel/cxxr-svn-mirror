/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2007 Andrew Runnalls.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street Fifth Floor, Boston, MA 02110-1301  USA
 */

/** @file GCRoot.cpp
 *
 * Implementation of class GCRootBase.
 */

#include "CXXR/GCRoot.h"

#include <stdexcept>

using namespace std;
using namespace CXXR;

// Force generation of non-inline embodiments of functions in the C
// interface:
namespace {
    RObject* (*protectp)(RObject*) = Rf_protect;
    void (*unprotectp)(int) = Rf_unprotect;
    void (*ProtectWithIndexp)(SEXP, PROTECT_INDEX *) = R_ProtectWithIndex;
    void (*Reprotectp)(SEXP, PROTECT_INDEX) = R_Reprotect;
}

vector<GCNode*> GCRootBase::s_roots;

#ifdef NDEBUG
vector<RObject*> GCRootBase::s_pps;
#else
vector<pair<RObject*, RCNTXT*> > GCRootBase::s_pps;
#endif

void GCRootBase::ppsRestoreSize(size_t new_size)
{
    if (new_size > s_pps.size())
	throw out_of_range("GCRootBase::ppsRestoreSize: requested size"
			   " greater than current size.");
    s_pps.resize(new_size);
}

void GCRootBase::reprotect(RObject* node, unsigned int index)
{
    if (index >= s_pps.size())
	throw out_of_range("GCRootBase::reprotect: index out of range.");
#ifdef NDEBUG
    s_pps[index] = node;
#else
    pair<RObject*, RCNTXT*>& pr = s_pps[index];
    if (pr.second != R_GlobalContext)
	throw logic_error("GCRootBase::reprotect: not in same context"
			  " as the corresponding call of protect().");
    pr.first = node;
#endif
}

void GCRootBase::seq_error()
{
    throw logic_error("GCRoots must be destroyed"
		      " in reverse order of creation\n");
}

void GCRootBase::unprotect(unsigned int count)
{
    size_t sz = s_pps.size();
    if (count > sz)
	throw out_of_range("GCRootBase::unprotect: count greater"
			   " than current stack size.");
#ifdef NDEBUG
    s_pps.resize(sz - count);
#else
    for (unsigned int i = 0; i < count; ++i) {
	const pair<RObject*, RCNTXT*>& pr = s_pps.back();
	if (pr.second != R_GlobalContext)
	    throw logic_error("GCRootBase::unprotect: not in same context"
			      " as the corresponding call of protect().");
	s_pps.pop_back();
    }
#endif
}

void GCRootBase::unprotectPtr(RObject* node)
{
#ifdef NDEBUG
    vector<RObject*>::reverse_iterator rit
	= find(s_pps.rbegin(), s_pps.rend(), node);
#else
    vector<pair<RObject*, RCNTXT*> >::reverse_iterator rit = s_pps.rbegin();
    while (rit != s_pps.rend() && (*rit).first != node)
	++rit;
#endif
    if (rit == s_pps.rend())
	throw invalid_argument("GCRootBase::unprotectPtr:"
			       " pointer not found.");
    // See Josuttis p.267 for the need for -- :
    s_pps.erase(--(rit.base()));
}

void GCRootBase::visitRoots(GCNode::const_visitor* v)
{
    for (vector<GCNode*>::iterator it = s_roots.begin();
	 it != s_roots.end(); ++it) {
	GCNode* n = *it;
	if (n) n->conductVisitor(v);
    }
#ifdef NDEBUG
    for (vector<RObject*>::iterator it = s_pps.begin();
	 it != s_pps.end(); ++it) {
	RObject* n = *it;
	if (n) n->conductVisitor(v);
    }
#else
    for (vector<pair<RObject*, RCNTXT*> >::iterator it = s_pps.begin();
	 it != s_pps.end(); ++it) {
	RObject* n = (*it).first;
	if (n) n->conductVisitor(v);
    }
#endif
}

void GCRootBase::visitRoots(GCNode::visitor* v)
{
    for (vector<GCNode*>::iterator it = s_roots.begin();
	 it != s_roots.end(); ++it) {
	GCNode* n = *it;
	if (n) n->conductVisitor(v);
    }
#ifdef NDEBUG
    for (vector<RObject*>::iterator it = s_pps.begin();
	 it != s_pps.end(); ++it) {
	RObject* n = *it;
	if (n) n->conductVisitor(v);
    }
#else
    for (vector<pair<RObject*, RCNTXT*> >::iterator it = s_pps.begin();
	 it != s_pps.end(); ++it) {
	RObject* n = (*it).first;
	if (n) n->conductVisitor(v);
    }
#endif
}

void Rf_ppsRestoreSize(size_t new_size)
{
    GCRootBase::ppsRestoreSize(new_size);
}

size_t Rf_ppsSize()
{
    return GCRootBase::ppsSize();
}