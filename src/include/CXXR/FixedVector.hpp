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

/** @file FixedVector.hpp
 *
 * @brief Class template CXXR::FixedVector.
 */

#ifndef FIXEDVECTOR_HPP
#define FIXEDVECTOR_HPP 1

#include <boost/aligned_storage.hpp>
#include "CXXR/VectorBase.h"

namespace CXXR {
    /** @brief R data vector primarily intended for fixed-size use.
     *
     * This is a general-purpose class template to represent an R data
     * vector, and is intended particularly for the case where the
     * size of the vector is fixed when it is constructed.
     *
     * Having said that, the template \e does implement setSize(),
     * primarily to service CR code's occasional use of SETLENGTH();
     * however, the implementation of this is rather inefficient.
     *
     * CXXR implements all of CR's built-in vector types using this
     * template.
     *
     * @tparam T The type of the elements of the vector.
     *
     * @tparam ST The required ::SEXPTYPE of the vector.
     *
     * @tparam Initializer (optional).  Class of function object
     *           defining <code>operator()(RObject*)</code>. (Any
     *           return value is discarded.)  When a FixedVector
     *           object is constructed, a default-constructed
     *           Initializer object is applied to it.  This can be
     *           used, for example, to apply an R class attribute
     *           etc.  The default is to do nothing.
     */
    template <typename T, SEXPTYPE ST,
	      typename Initializer /* = RObject::DoNothing */>
    class FixedVector : public VectorBase {
    public:
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;

	/** @brief Create a vector, leaving its contents
	 *         uninitialized (for POD types) or default
	 *         constructed.
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 */
	FixedVector(std::size_t sz)
	    : VectorBase(ST, sz), m_data(singleton())
	{
	    if (sz > 1)
		m_data = allocData(sz);
	    if (ElementTraits::MustConstruct<T>::value)  // known at compile-time
		constructElements(begin(), end());
	    Initializer()(this);
	}

	/** @brief Create a vector, and fill with a specified initial
	 *         value.
	 *
	 * @tparam U type assignable to \a T .
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 *
	 * @param fill_value Initial value to be assigned to every
	 *          element.
	 */
	template <typename U>
	FixedVector(std::size_t sz, const U& fill_value);

	/** @brief Copy constructor.
	 *
	 * @param pattern FixedVector to be copied.
	 */
	FixedVector(const FixedVector<T, ST, Initializer>& pattern);

	/** @brief Constructor from range.
	 * 
	 * @tparam An iterator type, at least a forward iterator.
	 *
	 * @param from Iterator designating the start of the range
	 *          from which the FixedVector is to be constructed.
	 *
	 * @param to Iterator designating 'one past the end' of the
	 *          range from which the FixedVector is to be
	 *          constructed.
	 */
	template <typename FwdIter>
	FixedVector(FwdIter from, FwdIter to);

	/** @brief Element access.
	 *
	 * @param index Index of required element (counting from
	 *          zero).  No bounds checking is applied.
	 *
	 * @return Reference to the specified element.
	 */
	T& operator[](unsigned int index)
	{
	    return m_data[index];
	}

	/** @brief Read-only element access.
	 *
	 * @param index Index of required element (counting from
	 *          zero).  No bounds checking is applied.
	 *
	 * @return \c const reference to the specified element.
	 */
	const T& operator[](unsigned int index) const
	{
	    return m_data[index];
	}

	/** @brief Iterator designating first element.
	 *
	 * @return An iterator designating the first element of the
	 * vector.  Returns end() if the vector is empty.
	 */
	iterator begin()
	{
	    return m_data;
	}

	/** @brief Const iterator designating first element.
	 *
	 * @return A const_iterator designating the first element of
	 * the vector.  Returns end() if the vector is empty.
	 */
	const_iterator begin() const
	{
	    return m_data;
	}

	/** @brief One-past-the-end iterator.
	 *
	 * @return An iterator designating a position 'one past the
	 * end' of the vector.
	 */
	iterator end()
	{
	    return begin() + size();
	}

	/** @brief One-past-the-end const_iterator.
	 *
	 * @return A const_iterator designating a position 'one past
	 * the end' of the vector.
	 */
	const_iterator end() const
	{
	    return begin() + size();
	}

	/** @brief Name by which this type is known in R.
	 *
	 * @return the name by which this type is known in R.
	 *
	 * @note This function is declared but not defined as part of
	 * the FixedVector template.  It must be defined as a
	 * specialization for each instantiation of the template for
	 * which it or typeName() is used.
	 */
	static const char* staticTypeName();

	// Virtual functions of VectorBase:
	void setSize(std::size_t new_size);

	// Virtual functions of RObject:
	FixedVector<T, ST, Initializer>* clone() const;
	const char* typeName() const;

	// Virtual function of GCNode:
	void visitReferents(const_visitor* v) const;
    protected:
	/**
	 * Declared protected to ensure that FixedVector objects are
	 * allocated only using 'new'.
	 */
	~FixedVector()
	{
	    if (ElementTraits::MustDestruct<T>::value)  // known at compile-time
		destructElements();
	    if (m_data != singleton())
		MemoryBank::deallocate(m_data, size()*sizeof(T));
	}

	// Virtual function of GCNode:
	void detachReferents();
    private:
	T* m_data;  // pointer to the vector's data block.

	// If there is only one element, it is stored here, internally
	// to the FixedVector object, rather than via a separate
	// allocation from CXXR::MemoryBank.  We put this last, so
	// that it will be adjacent to any trailing redzone.  Note
	// that if a FixedVector is *resized* to 1, its data is held
	// in a separate memory block, not here.
	boost::aligned_storage<sizeof(T), boost::alignment_of<T>::value>
	m_singleton_buf;

	// Not implemented yet.  Declared to prevent
	// compiler-generated versions:
	FixedVector& operator=(const FixedVector&);

	// If there is more than one element, this function is used to
	// allocate the required memory block from CXXR::MemoryBank :
	static T* allocData(std::size_t sz);

	static void constructElements(iterator from, iterator to);

	void destructElements();

	// Helper function for detachReferents():
	void detachElements();

	T* singleton()
	{
	    return static_cast<T*>(static_cast<void*>(&m_singleton_buf));
	}

	// Helper function for visitReferents():
	void visitElements(const_visitor* v) const;
    };
}  // namespace CXXR


// ***** Implementation of non-inlined members *****

#include <algorithm>
#include "localization.h"
#include "R_ext/Error.h"

template <typename T, SEXPTYPE ST, typename Initr>
template <typename U>
CXXR::FixedVector<T, ST, Initr>::FixedVector(std::size_t sz,
						   const U& fill_value)
    : VectorBase(ST, sz), m_data(singleton())
{
    if (sz > 1)
	m_data = allocData(sz);
    for (T *p = m_data, *pend = m_data + sz; p != pend; ++p)
	new (p) T(fill_value);
    Initr()(this);
}

template <typename T, SEXPTYPE ST, typename Initr>
CXXR::FixedVector<T, ST, Initr>::FixedVector(const FixedVector<T, ST, Initr>& pattern)
    : VectorBase(pattern), m_data(singleton())
{
    std::size_t sz = size();
    if (sz > 1)
	m_data = allocData(sz);
    T* p = m_data;
    for (const_iterator it = pattern.begin(), end = pattern.end();
	 it != end; ++it)
	new (p++) T(*it);
    Initr()(this);
}

template <typename T, SEXPTYPE ST, typename Initr>
template <typename FwdIter>
CXXR::FixedVector<T, ST, Initr>::FixedVector(FwdIter from, FwdIter to)
    : VectorBase(ST, std::distance(from, to)), m_data(singleton())
{
    if (size() > 1)
	m_data = allocData(size());
    T* p = m_data;
    for (const_iterator it = from; it != to; ++it)
	new (p++) T(*it);
    Initr()(this);
}

template <typename T, SEXPTYPE ST, typename Initr>
T* CXXR::FixedVector<T, ST, Initr>::allocData(std::size_t sz)
{
    std::size_t blocksize = sz*sizeof(T);
    // Check for integer overflow:
    if (blocksize/sizeof(T) != sz)
	Rf_error(_("request to create impossibly large vector."));
    return static_cast<T*>(MemoryBank::allocate(blocksize));
}

template <typename T, SEXPTYPE ST, typename Initr>
CXXR::FixedVector<T, ST, Initr>* CXXR::FixedVector<T, ST, Initr>::clone() const
{
    // Can't use CXXR_NEW because the comma confuses GNU cpp:
    return expose(new FixedVector<T, ST, Initr>(*this));
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::constructElements(iterator from,
							iterator to)
{
    for (iterator p = from; p != to; ++p)
	new (p) T;
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::destructElements()
{
    // Destroy in reverse order, following C++ convention:
    for (T* p = m_data + size() - 1; p >= m_data; --p)
	p->~T();
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::detachElements()
{
    std::for_each(begin(), end(), ElementTraits::DetachReferents<T>());
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::detachReferents()
{
    if (ElementTraits::HasReferents<T>::value)  // known at compile-time
	detachElements();
    VectorBase::detachReferents();
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::setSize(std::size_t new_size)
{
    std::size_t copysz = std::min(size(), new_size);
    T* newblock = singleton();  // Setting used only if new_size == 0
    if (new_size > 0)
	newblock = allocData(new_size);
    // The following is essential for e.g. RHandles, otherwise they
    // may contain junk pointers.
    if (ElementTraits::MustConstruct<T>::value)  // known at compile time
	constructElements(newblock, newblock + copysz);
    T* p = std::copy(begin(), begin() + copysz, newblock);
    T* newblockend = newblock + new_size;
    for (; p != newblockend; ++p)
	new (p) T(NA<T>());
    if (ElementTraits::MustDestruct<T>::value)  // known at compile-time
	destructElements();
    if (m_data != singleton())
	MemoryBank::deallocate(m_data, size()*sizeof(T));
    m_data = newblock;
    adjustSize(new_size);
}

template <typename T, SEXPTYPE ST, typename Initr>
const char* CXXR::FixedVector<T, ST, Initr>::typeName() const
{
    return FixedVector<T, ST, Initr>::staticTypeName();
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::visitElements(const_visitor* v) const
{
    std::for_each(begin(), end(), ElementTraits::VisitReferents<T>(v));
}

template <typename T, SEXPTYPE ST, typename Initr>
void CXXR::FixedVector<T, ST, Initr>::visitReferents(const_visitor* v) const
{
    if (ElementTraits::HasReferents<T>::value)  // known at compile-time
	visitElements(v);
    VectorBase::visitReferents(v);
}

#endif  // FIXEDVECTOR_HPP
