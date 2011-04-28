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
	typedef T element_type;
	typedef T* iterator;
	typedef const T* const_iterator;

	/** @brief Create a vector, leaving its contents
	 *         uninitialized.
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 */
	FixedVector(size_t sz)
	    : VectorBase(ST, sz), m_data(singleton()), m_blocksize(0)
	{
	    if (sz > 1)
		allocData(sz);
	    constructElements(typename ElementTraits<T>::MustConstruct::TruthType());
	}

	/** @brief Create a vector, and fill with a specified initial
	 *         value.
	 *
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 *
	 * @param initializer Initial value to be assigned to every
	 *          element.
	 */
	FixedVector(size_t sz, const T& initializer);

	/** @brief Copy constructor.
	 *
	 * @param pattern FixedVector to be copied.
	 */
	FixedVector(const FixedVector<T, ST>& pattern);

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
	void setSize(size_t new_size);

	// Virtual functions of RObject:
	FixedVector<T, ST>* clone() const;
	const char* typeName() const;

	// Virtual function of GCNode:
	//void visitReferents(const_visitor* v) const
	//{}  // FIXME
    protected:
	/**
	 * Declared protected to ensure that FixedVector objects are
	 * allocated only using 'new'.
	 */
	~FixedVector()
	{
	    destructElements(0,
			     typename ElementTraits<T>::MustDestruct::TruthType());
	    if (m_blocksize > 0)
		MemoryBank::deallocate(m_data, m_blocksize);
	}

	// Virtual function of GCNode:
	//void detachReferents()
	//{}  // FIXME
    private:
	T* m_data;  // pointer to the vector's data block.
	size_t m_blocksize;  // size of externally allocated data
		 // block, or zero if this is a singleton.

	// If there is only one element, it is stored here, internally
	// to the FixedVector object, rather than via a separate
	// allocation from CXXR::MemoryBank.  We put this last, so
	// that it will be adjacent to any trailing redzone.
	char m_singleton_buf[sizeof(T)];

	// Not implemented yet.  Declared to prevent
	// compiler-generated versions:
	FixedVector& operator=(const FixedVector&);

	// If there is more than one element, this function is used to
	// allocate the required memory block from CXXR::MemoryBank :
	void allocData(size_t sz);

	void constructElements(False)
	{}

	void constructElements(True);

	void destructElements(size_t, False)
	{}

	// Must have new_size <= current size.
	void destructElements(size_t new_size, True);

	// Helper functions for detachReferents():
	void detachElements(False)
	{}

	void detachElements(True);

	T* singleton()
	{
	    return reinterpret_cast<T*>(m_singleton_buf);
	}

	// Helper functions for visitReferents():
	void visitElements(const_visitor*, False)
	{}

	void visitElements(const_visitor* v, True);
    };
}  // namespace CXXR


// ***** Implementation of non-inlined members *****

#include "localization.h"
#include "R_ext/Error.h"

template <typename T, SEXPTYPE ST>
CXXR::FixedVector<T, ST>::FixedVector(size_t sz, const T& initializer)
    : VectorBase(ST, sz), m_data(singleton()), m_blocksize(0)
{
    if (sz > 1)
	allocData(sz);
    constructElements(typename ElementTraits<T>::MustConstruct::TruthType());
    std::fill(begin(), end(), initializer);
}

template <typename T, SEXPTYPE ST>
CXXR::FixedVector<T, ST>::FixedVector(const FixedVector<T, ST>& pattern)
    : VectorBase(pattern), m_data(singleton()), m_blocksize(0)
{
    size_t sz = size();
    if (sz > 1)
	allocData(sz);
    constructElements(typename ElementTraits<T>::MustConstruct::TruthType());
    std::copy(pattern.begin(), pattern.end(), begin());
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::allocData(size_t sz)
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
void CXXR::FixedVector<T, ST>::constructElements(True)
{
    iterator pend = end();
    for (iterator p = begin(); p < pend; ++p)
	new (p) T;
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::destructElements(size_t new_size, True)
{
    // Destroy in reverse order, following C++ convention:
    for (T* p = m_data + size() - 1; p >= m_data + new_size; --p)
	p->~T();
}

template <typename T, SEXPTYPE ST>
void CXXR::FixedVector<T, ST>::setSize(size_t new_size)
{
    if (new_size > size())
	Rf_error("cannot increase size of this vector");
    destructElements(new_size,
		     typename ElementTraits<T>::MustDestruct::TruthType());
    VectorBase::setSize(new_size);
}

template <typename T, SEXPTYPE ST>
const char* CXXR::FixedVector<T, ST>::typeName() const
{
    return FixedVector<T, ST>::staticTypeName();
}

#endif  // FIXEDVECTOR_HPP
