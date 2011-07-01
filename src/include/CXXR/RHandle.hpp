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

/** @file RHandle.hpp
 *
 * @brief Class template CXXR::RHandle.
 */

#ifndef RHANDLE_HPP
#define RHANDLE_HPP

#include "CXXR/ElementTraits.hpp"
#include "CXXR/GCEdge.hpp"

#define LAZYCOPY

namespace CXXR {
    class RObject;

    /** @brief Untemplated base class for RHandle<T>.
     */
    class RHandleBase : public GCEdgeBase {
    public:
	~RHandleBase();
    protected:
	RHandleBase(const RObject* target = 0);

	const RObject* asset() const;

	bool sharing() const;
    private:
	// Not implemented:
	RHandleBase(const RHandleBase&);
	RHandleBase& operator=(const RHandleBase&);
    };
	
    /** @brief Smart pointer used to control the copying of RObjects.
     *
     * This class encapsulates a T* pointer, where T is derived from
     * RObject, and is used to manage the copying of subobjects when
     * an RObject is copied.  For most purposes, it behaves
     * essentially like a GCEdge<T>.  However, when a RHandle is
     * copied, it checks whether the object, \a x say, that it points
     * to is clonable.  If it is, then the copied RHandle will point to
     * a clone of \a x ; if not, then the copy will point to \a x
     * itself.
     *
     * @param T RObject or a class publicly derived from RObject.
     */
    template <class T = RObject>
    class RHandle : public RHandleBase {
    public:
	RHandle()
	{}

	/** @brief Primary constructor.
	 *
	 * @param target Pointer to the object to which this
	 *          RHandle is to refer.
	 */
	explicit RHandle(const T* target)
	    : RHandleBase(target)
	{}

	/** @brief Copy constructor.
	 *
	 * @param source Reference to RHandle to be copied.
	 */
	RHandle(const RHandle& source)
	    : RHandleBase(source.get())
	{}

	/** @brief Assignment operator.
	 */
	RHandle<T>& operator=(const RHandle<T>& source)
	{
	    if (source.asset() != asset()) {
		RHandle<T> tmp(source);
		swap(tmp);
	    }
	    return *this;
	}

	/** @brief Assignment from pointer.
	 *
	 * Note that this does not attempt to clone \a newtarget: it
	 * merely changes this RHandle to point to \a newtarget.
	 */
	RHandle<T>& operator=(const T* newtarget)
	{
	    if (newtarget != asset()) {
		RHandle<T> tmp(newtarget);
		swap(tmp);
	    }
	    return *this;
	}

	const T* operator->() const
	{
	    return get();
	}

	T* operator->()
	{
	    return get();
	}

	/** @brief Access the encapsulated pointer (const form).
	 *
	 * @return the encapsulated pointer.
	 */
	operator const T*() const
	{
	    return get();
	}

	/** @brief Access the encapsulated pointer.
	 *
	 * This function clones the target of the pointer if it is shared.
	 *
	 * @return the encapsulated pointer.
	 */
	T* get()
	{
#ifdef LAZYCOPY
	    if (sharing()) {
		GCNode::GCInhibitor inh;
		T* clone = static_cast<T*>(asset()->clone());
		if (clone)
		    operator=(clone);
	    }
#endif
	    return static_cast<T*>(const_cast<RObject*>(asset()));
	}

	/** @brief Access the encapsulated pointer (const variant).
	 *
	 * @return the encapsulated pointer.
	 */
	const T* get() const
	{
	    return static_cast<const T*>(target());
	}

	/** @brief Swap target with another RHandle<T>.
	 *
	 * @param that Reference to the RHandle<T> with which targets are
	 *          to be swapped.
	 */

	void swap(RHandle<T>& that)
	{
	    GCEdgeBase::swap(that);
	}
    };  // class template RHandle

    // Partial specializations of ElementTraits:
    namespace ElementTraits {
	template <class T>
	struct DetachReferents<RHandle<T> >
	    : std::unary_function<T, void> {
	    void operator()(RHandle<T>& t) const
	    {
		t = 0;
	    }
	};

	template <class T>
	struct HasReferents<RHandle<T> > : boost::mpl::true_
	{};

	template <class T>
	struct MustConstruct<RHandle<T> > : boost::mpl::true_
	{};

	template <class T>
	struct MustDestruct<RHandle<T> >  : boost::mpl::true_
	{};

	template <class T>
	class VisitReferents<RHandle<T> >
	    : public std::unary_function<T, void> {
	public:
	    VisitReferents(GCNode::const_visitor* v)
		: m_v(v)
	    {}

	    void operator()(const RHandle<T>& t) const
	    {
		if (t)
		    (*m_v)(t);
	    }
	private:
	    GCNode::const_visitor* m_v;
	};

	template <class T>
	struct NAFunc<RHandle<T> > {
	    const RHandle<T>& operator()() const
	    {
		static RHandle<T> na;
		return na;
	    }
	};

	template <class T>
	struct IsNA<RHandle<T> > {
	    bool operator()(const RHandle<T>& t) const
	    {
		return false;
	    }
	};
    }  // namespace ElementTraits
}  // namespace CXXR

#endif // RHANDLE_HPP
