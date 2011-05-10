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

/** @file StdFrame.hpp
 *
 * @brief Class CXXR::StdFrame.
 */

#ifndef STDFRAME_HPP
#define STDFRAME_HPP

#include <tr1/unordered_map>
#include "CXXR/Allocator.hpp"
#include "CXXR/Frame.hpp"

namespace CXXR {
    /** @brief General-purpose implementation of CXXR::Frame.
     */
    class StdFrame : public Frame {
    private:
	typedef
	std::tr1::unordered_map<const Symbol*, Binding,
				std::tr1::hash<const Symbol*>,
				std::equal_to<const Symbol*>,
				CXXR::Allocator<std::pair<const Symbol* const,
							  Binding> >
	                        > map;
    public:
	/**
	 * @param initial_capacity A hint to the implementation that
	 *          the constructed StdFrame should be
	 *          configured to have capacity for at least \a
	 *          initial_capacity Bindings.  This does not impose an
	 *          upper limit on the capacity of the StdFrame,
	 *          but some reconfiguration (and consequent time
	 *          penalty) may occur if it is exceeded.
	 */
	explicit StdFrame(std::size_t initial_capacity = 15);
	// Why 15?  Because if the implementation uses a prime number
	// hash table sizing policy, this will result in the
	// allocation of a hash table array comprising 31 buckets.  On
	// a 32-bit architecture, this will fit well into two 64-byte
	// cache lines.

	// Virtual functions of Frame (qv):
	PairList* asPairList() const;

#ifdef __GNUG__
	__attribute__((hot,fastcall))
#endif
	Binding* binding(const Symbol* symbol);

	const Binding* binding(const Symbol* symbol) const;
	void clear();
	StdFrame* clone() const;
	bool erase(const Symbol* symbol);
	void lockBindings();
	std::size_t numBindings() const;
	Binding* obtainBinding(const Symbol* symbol);
	std::size_t size() const;
	void softMergeInto(Frame* target) const;
	std::vector<const Symbol*> symbols(bool include_dotsymbols) const;

	// Virtual function of GCNode:
	void visitReferents(const_visitor* v) const;
    protected:
	// Virtual function of GCNode:
	void detachReferents();
    private:
	map m_map;

	// Declared private to ensure that StdFrame objects are
	// created only using 'new':
	~StdFrame() {}

	// Not (yet) implemented.  Declared to prevent
	// compiler-generated versions:
	StdFrame& operator=(const StdFrame&);
    };
}  // namespace CXXR

#endif // STDFRAME_HPP
