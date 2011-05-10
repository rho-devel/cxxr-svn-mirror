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
    /** @brief R data vector whose capacity is fixed at construction.
     *
     * This is a general-purpose class template to represent an R data
     * vector.  The capacity of the vector is fixed when it is
     * constructed.
     *
     * @tparam T The type of the elements of the vector.
     *
     * @tparam ST The required ::SEXPTYPE of the vector.
     */
    template <typename T, SEXPTYPE ST>
    class FixedVector : public VectorBase {
    public:
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;

	/** @brief Create a vector, leaving its contents
	 *         uninitialized.
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 */
	FixedVector(std::size_t sz)
	    : VectorBase(ST, sz), m_data(singleton()), m_blocksize(0)
	{
	    if (sz > 1)
		allocData(sz);
	    if (ElementTraits::MustConstruct<T>())  // determined at compile-time
		constructElements();
	}

	/** @brief Create a vector, and fill with a specified initial
	 *         value.
	 *
	 * @tparam U type assignable to \a T .
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 *
	 * @param initializer Initial value to be assigned to every
	 *          element.
	 */
	template <typename U>
	FixedVector(std::size_t sz, const U& initializer);

	/** @brief Copy constructor.
	 *
	 * @param pattern FixedVector to be copied.
	 */
	FixedVector(const FixedVector<T, ST>& pattern);

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

	/** @brief Adjust the number of elements in the vector.
	 *
	 * @param new_size New size required.  Zero is permissible,
	 *          but the new size must not be greater than the
	 *          current size.
	 *
	 * @deprecated The facility to resize FixedVector object may
	 * be withdrawn in future.  Currently used by SETLENGTH()
	 * (which itself is little used).
	 */
	void setSize(std::size_t new_size);

	// Virtual functions of RObject:
	FixedVector<T, ST>* clone() const;
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
	    if (ElementTraits::MustDestruct<T>())  // determined at compile-time
		destructElements(0);
	    if (m_blocksize > 0)
		MemoryBank::deallocate(m_data, m_blocksize);
	}

	// Virtual function of GCNode:
	void detachReferents();
    private:
	T* m_data;  // pointer to the vector's data block.
	std::size_t m_blocksize;  // size of externally allocated data
		      // block, or zero if this is a singleton.

	// If there is only one element, it is stored here, internally
	// to the FixedVector object, rather than via a separate
	// allocation from CXXR::MemoryBank.  We put this last, so
	// that it will be adjacent to any trailing redzone.
	boost::aligned_storage<sizeof(T), boost::alignment_of<T>::value>
	m_singleton_buf;

	// Not implemented yet.  Declared to prevent
	// compiler-generated versions:
	FixedVector& operator=(const FixedVector&);

	// If there is more than one element, this function is used to
	// allocate the required memory block from CXXR::MemoryBank :
	void allocData(std::size_t sz);

	void constructElements();

	// Must have new_size <= current size.
	void destructElements(std::size_t new_size);

	// Helper functions for detachReferents():
	void detachElements();

	T* singleton()
	{
	    return static_cast<T*>(static_cast<void*>(&m_singleton_buf));
	}

	// Helper functions for visitReferents():
	void visitElements(const_visitor* v) const;
    };
}  // namespace CXXR


// ***** Implementation of non-inlined members *****

#include <algorithm>
#include "localization.h"
#include "R_ext/Error.h"

template <typename T, SEXPTYPE ST>
template <typename U>
CXXR::FixedVector<T, ST>::FixedVector(std::size_t sz, const U& initializer)
    : VectorBase(ST, sz), m_data(singleton()), m_blocksize(0)
{
    if (sz > 1)
	allocData(sz);
    if (ElementTraits::MustConstruct<T>())  // determined at compile-time
	constructElements();
    std::fill(begin(), end(), initializer);
}

template <typename T, SEXPTYPE ST>
CXXR::FixedVector<T, ST>::FixedVector(const FixedVector<T, ST>& pattern)
    : VectorBase(pattern), m_data(singleton()), m_blocksize(0)
{
    std::size_t sz = size();
    if (sz > 1)
	allocData(sz);
    if (ElementTraits::MustConstruct<T>())  // determined at compile-time
	constructElements();
    std::copy(pattern.begin(), pattern.end(), begin());
}

template <typename T, SEXPTYPE ST>
template <typename FwdIter>
CXXR::FixedVector<T, ST>::FixedVector(FwdIter from, FwdIter to)
    : VectorBase(ST, std::distance(from, to)), m_data(singleton()),
      m_blocksize(0)
{
    if (size() > 1)
	allocData(size());
    if (ElementTraits::MustConstruct<T>())  // determined at compile-time
	constructElements();
    std::copy(from, to, begin());
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::allocData(std::size_t sz)
{
    m_blocksize = sz*sizeof(T);
    // Check for integer overflow:
    if (m_blocksize/sizeof(T) != sz)
	Rf_error(_("request to create impossibly large vector."));
    m_data = static_cast<T*>(MemoryBank::allocate(m_blocksize));
}

template <typename T, SEXPTYPE ST>
CXXR::FixedVector<T, ST>* CXXR::FixedVector<T, ST>::clone() const
{
    // Can't use CXXR_NEW because the comma confuses GNU cpp:
    return expose(new FixedVector<T, ST>(*this));
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::constructElements()
{
    for (iterator p = begin(), pend = end(); p != pend; ++p)
	new (p) T;
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::destructElements(std::size_t new_size)
{
    // Destroy in reverse order, following C++ convention:
    for (T* p = m_data + size() - 1; p >= m_data + new_size; --p)
	p->~T();
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::detachElements()
{
    //std::for_each(begin(), end(), ElementTraits::DetachReferents<T>());
    setSize(0);
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::detachReferents()
{
    if (ElementTraits::HasReferents<T>())  // determined at compile-time
	detachElements();
    VectorBase::detachReferents();
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::setSize(std::size_t new_size)
{
    if (new_size > size())
	Rf_error("cannot increase size of this vector");
    if (ElementTraits::MustDestruct<T>())  // determined at compile-time
	destructElements(new_size);
    VectorBase::setSize(new_size);
}

template <typename T, SEXPTYPE ST>
const char* CXXR::FixedVector<T, ST>::typeName() const
{
    return FixedVector<T, ST>::staticTypeName();
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::visitElements(const_visitor* v) const
{
    std::for_each(begin(), end(), ElementTraits::VisitReferents<T>(v));
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::visitReferents(const_visitor* v) const
{
    if (ElementTraits::HasReferents<T>())  // determined at compile-time
	visitElements(v);
    VectorBase::visitReferents(v);
}

#endif  // FIXEDVECTOR_HPP
