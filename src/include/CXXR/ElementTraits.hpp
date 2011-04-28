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

/** @file ElementTraits.hpp
 *
 * @brief Class and supporting functions encapsulating traits of R
 * vector element types.
 */

#ifndef ELEMENTTRAITS_HPP
#define ELEMENTTRAITS_HPP 1

namespace CXXR {
    struct True {};

    struct False {};

    /** @brief Class encapsulating traits of R vector element types.
     *
     * This templated class, all of whose members are static, is used
     * to record characteristics of types capable of being used as the
     * elements of R data vectors, to facilitate the writing of generic
     * algorithms manipulating such vectors.
     *
     * @tparam T A type capable of being used as the element type of an
     *           R data vector.
     */
    template <typename T>
    struct ElementTraits {
	/** @brief Information about the data payload.
	 *
	 * In some element types, including all the standard R atomic
	 * data types, the 'value' of a vector element is held
	 * directly in a data item of the element type \a T , and in
	 * that case a special value within the range of type \a T may
	 * be used to signify that an item of data is 'not available'.
	 *
	 * However, CXXR also allows the possibility that a vector
	 * element type \a T can be a class type whose objects contain
	 * a value of some underlying type, representing the data
	 * 'payload', along with a separate flag (typically a bool)
	 * indicating whether or not the data is 'not available'.
	 *
	 * This class provides facilities to allow generic programs to
	 * handle both these cases straightforwardly.  As defined
	 * here, the class deals with the first case described above;
	 * specializations of the ElementTraits template can be used
	 * to address the second case.
	 */
	struct Data {
	    /** @brief Type of the data payload held in this element
	     * type.
	     */
	    typedef T Type;

	    /** @brief Access the data payload.
	     *
	     * @param t Reference to an object of the element type.
	     *
	     * @return reference to the data payload contained within
	     * \a t .
	     */
	    static const Type& get(const T& t)
	    {
		return t;
	    }
	};  // struct Data

	/** @brief Do elements of this type require construction?
	 *
	 * Specializations will define \c MustConstruct::TruthType to
	 * be True if element type \a T has a nontrivial default
	 * constructor.
	 *
	 * In the default case, covered here, \c
	 * MustConstruct::TruthType is defined to False, signifying
	 * that no construction is required.
	 */
	struct MustConstruct {
	    typedef False TruthType;
	};

	/** @brief Does this type have a destructor?
	 *
	 * Specializations will define \c MustDestruct::TruthType to
	 * be True if element type \a T has a nontrivial destructor.
	 *
	 * In the default case, covered here, \c
	 * MustDestruct::TruthType is defined to False, signifying
	 * that no destructor call is required.
	 */
	struct MustDestruct {
	    typedef False TruthType;
	};

	/** @brief Value to be used if 'not available'.
	 *
	 * @return The value of this type to be used if the actual
	 * value is not available.  This for example is the value that
	 * will be used if an element of a vector of \a T is accessed
	 * in R using a NA index.
	 *
	 * @note For some types, e.g. Rbyte, the value returned is not
	 * distinct from ordinary values of the type.  See
	 * hasDistinctNA().
	 */
	static const T& NA();

	/** @brief Does a type have a distinct 'not available' value?
	 *
	 * @return true iff the range of type \a T includes a distinct
	 * value to signify that the actual value of the quantity is not
	 * available.
	 */
	inline static bool hasDistinctNA()
	{
	    return isNA(NA());
	}

	/** @brief Does a value represent a distinct 'not available'
	 *  status?
	 *
	 * @param t A value of type \a T .
	 *
	 * @return true iff \a t has a distinct value (or possibly,
	 * one of a set of distinct values) signifying that the actual
	 * value of this quantity is not available.
	 */
	inline static bool isNA(const T& t)
	{
	    return t == NA();
	}
    };  // struct ElementTraits

    /** @brief Access the data payload of an R vector element.
     *
     * This templated function is syntactic sugar for the Data::get()
     * function of the ElementTraits class template.  It should not be
     * specialized: instead specialize ElementTraits itself.
     *
     * @tparam T type used as an element in the CXXR implementation of
     *           an R vector type.
     *
     * @param t Reference to an object of type \a T .
     *
     * @return reference to the data payload contained within \a t .
     */
    template <typename T>
    inline const typename ElementTraits<T>::Data::Type&
    elementData(const T& t)
    {
	return ElementTraits<T>::Data::get(t);
    }

    /** @brief Does a value represent a distinct 'not available'
     *  status?
     *
     * This templated function is syntactic sugar for the isNA()
     * method of the ElementTraits class template.  It should not be
     * specialized: instead specialize ElementTraits itself.
     *
     * @tparam T type used as an element in the CXXR implementation of
     *           an R vector type.
     *
     * @param t A value of type \a T .
     *
     * @return true iff \a t has a distinct value (or possibly, one of
     * a set of distinct values) signifying that the actual value of
     * this quantity is not available.
     */
    template <typename T>
    inline bool isNA(const T& t)
    {
	return ElementTraits<T>::isNA(t);
    }

}  // namespace CXXR;

#endif  // ELEMENTTRAITS_HPP
