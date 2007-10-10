/*
 *  R : A Computer Language for Statistical Data Analysis
 * (C) Copyright Nicolai M. Josuttis 1999.
 *  Copyright (C) 2007  Andrew Runnalls
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* The following code is adapted from an example taken from the book
 * "The C++ Standard Library - A Tutorial and Reference"
 * by Nicolai M. Josuttis, Addison-Wesley, 1999
 *
 * (C) Copyright Nicolai M. Josuttis 1999.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 */

#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP 1

#include <limits>
#include "CXXR/Heap.hpp"

namespace CXXR {
    /** STL-compatible allocator front-ending CXXR::Heap.
     *
     * This templated class enables container classes within the C++
     * standard library to allocate their memory via CXXR::Heap.  It
     * is adapted from an example in the book "The C++ Standard
     * Library - A Tutorial and Reference" by Nicolai M. Josuttis,
     * Addison-Wesley, 1999.  Also see Item 10 of Meyers' 'Effective
     * STL' for the arcana of STL allocators.
     */
    template <class T>
    class Allocator {
    public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind {
	    typedef Allocator<U> other;
	};

	// return address of values
	pointer address (reference value) const {
	    return &value;
	}
	const_pointer address (const_reference value) const {
	    return &value;
	}

	/* constructors and destructor
	 * - nothing to do because the allocator has no state
	 */
	Allocator() throw() {
	}
	Allocator(const Allocator&) throw() {
	}
	template <class U>
	Allocator (const Allocator<U>&) throw() {
	}
	~Allocator() throw() {
	}

	// return maximum number of elements that can be allocated
	size_type max_size () const throw() {
	    return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	// allocate but don't initialize num elements of type T
	pointer allocate (size_type num, const void* /*hint*/ = 0) {
	    return reinterpret_cast<pointer>(Heap::allocate(num*sizeof(T)));
	}

	// initialize elements of allocated storage p with value value
	void construct (pointer p, const T& value) {
	    // initialize memory with placement new
	    new (p) T(value);
	}

	// destroy elements of initialized storage p
	void destroy (pointer p) {
	    // destroy objects by calling their destructor
	    p->~T();
	}

	// deallocate storage p of deleted elements
	void deallocate (pointer p, size_type num) {
	    Heap::deallocate(p, num*sizeof(T));
	}
    };

    // return that all specializations of this allocator are interchangeable
    template <class T1, class T2>
    bool operator== (const Allocator<T1>&,
		     const Allocator<T2>&) throw() {
	return true;
    }
    template <class T1, class T2>
    bool operator!= (const Allocator<T1>&,
		     const Allocator<T2>&) throw() {
	return false;
    }
}

#endif  // ALLOCATOR_HPP