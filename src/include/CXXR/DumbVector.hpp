/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2008  Andrew Runnalls
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

/** @file DumbVector.hpp
 *
 * @brief Templated class DumbVector.
 */

#ifndef DUMBVECTOR_HPP
#define DUMBVECTOR_HPP 1

#include "localization.h"
#include "R_ext/Error.h"
#include "CXXR/GCRoot.h"
#include "CXXR/VectorBase.h"

namespace CXXR {
    /** @brief Vector of 'plain old data'.
     *
     * This is a templated class to represent an R data vector.
     * @param T The type of the elements of the vector.  This should
     *          be either (i) a C++ built-in data type, or (ii) a
     *          class type with a trivial destructor, and a copy
     *          constructor implemented by bitwise copy.
     * @param ST The required \c SEXPTYPE of the vector.
     */
    template <class T, SEXPTYPE ST>
    class DumbVector : public VectorBase {
    public:
	/** @brief Create a vector, leaving its contents
	 *         uninitialized. 
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 */
	DumbVector(size_t sz)
	    : VectorBase(ST, sz), m_data(&m_singleton)
	{
	    if (sz > 1) allocData(sz);
#if VALGRIND_LEVEL >= 1
	    else VALGRIND_MAKE_MEM_UNDEFINED(&m_singleton, sizeof(T));
#endif
	}

	/** @brief Create a vector, and fill with a specified initial
	 *         value. 
	 * @param sz Number of elements required.  Zero is
	 *          permissible.
	 * @param initializer Initial value to be assigned to every
	 *          element.
	 */
	DumbVector(size_t sz, const T& initializer)
	    : VectorBase(ST, sz), m_data(&m_singleton),
	      m_singleton(initializer)
	{
	    if (sz > 1) allocData(sz, true);
	}

	/** @brief Element access.
	 * @param index Index of required element (counting from
	 *          zero).  No bounds checking is applied.
	 * @return Reference to the specified element.
	 */
	T& operator[](unsigned int index)
	{
	    return m_data[index];
	}

	/** @brief Read-only element access.
	 * @param index Index of required element (counting from
	 *          zero).  No bounds checking is applied.
	 * @return \c const reference to the specified element.
	 */
	const T& operator[](unsigned int index) const
	{
	    return m_data[index];
	}

	/**
	 * @return the name by which this type is known in R.
	 *
	 * @note This function is declared but not defined as part of
	 * the DumbVector template.  It must be defined as a
	 * specialization for each instantiation of the template for
	 * which it or typeName() is used.
	 */
	inline static const char* staticTypeName();

	// Virtual function of RObject:
	const char* typeName() const;
    protected:
	// Declared protected to ensure that DumbVectors are
	// allocated only using 'new'.
	~DumbVector()
	{
	    if (m_data != &m_singleton)
		Heap::deallocate(m_data, m_databytes);
	}
    private:
	size_t m_databytes;  // used only if > 1 elements
	T* m_data;  // pointer to the vector's data block.

	// If there is only one element, it is stored here, internally
	// to the DumbVector object, rather than via a separate
	// allocation from CXXR::Heap.  We put this last, so that it
	// will be adjacent to any trailing redzone.
	T m_singleton;

	// Not implemented yet.  Declared to prevent
	// compiler-generated versions:
	DumbVector(const DumbVector&);
	DumbVector& operator=(const DumbVector&);

	// If there is more than one element, this function is used to
	// allocate the required memory block from CXXR::Heap :
	void allocData(size_t sz, bool initialize = false);
    };

    template <class T, SEXPTYPE ST>
    void DumbVector<T, ST>::allocData(size_t sz, bool initialize)
    {
	m_databytes = sz*sizeof(T);
	// Check for integer overflow:
	if (m_databytes/sizeof(T) != sz)
	    Rf_error(_("Request to create impossibly large vector."));
	GCRoot<> thisroot(this);
	m_data = reinterpret_cast<T*>(Heap::allocate(m_databytes));
	if (initialize) {
	    for (unsigned int i = 0; i < sz; ++i)
		m_data[i] = m_singleton;
	}
#if VALGRIND_LEVEL == 1
	// For VALGRIND_LEVEL > 1 this will already have been done:
	else VALGRIND_MAKE_MEM_UNDEFINED(m_data, m_databytes);
#endif
    }

    template <class T, SEXPTYPE ST>
    const char* DumbVector<T, ST>::typeName() const
    {
	return DumbVector<T, ST>::staticTypeName();
    }
}  // namespace CXXR

#endif  // DUMBVECTOR_HPP