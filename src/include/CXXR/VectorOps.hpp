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

/** @file VectorOps.hpp
 *
 * @brief Functionality to support common operations on R vectors,
 * matrices and arrays.
 */

#ifndef VECTOROPS_HPP
#define VECTOROPS_HPP 1

namespace CXXR {
   /** @brief Services to support common operations on R vectors and arrays.
     *
     * This class, all of whose members are static, encapsulates
     * services supporting various commonly occurring operations on R
     * vector objects, including R matrices and arrays.
     */
    class VectorOps {
    public:
	/** @brief Control attribute copying for VectorOps::unary().
	 *
	 * An object from an instantiation of this template is used by
	 * VectorOps::unary() to determine which attributes are copied
	 * from the input vector to the output vector.  The default
	 * behaviour, implemented here, is to copy all attributes
	 * across, along with the S4 object status.  However, this can
	 * be overridden by specializing this template.
	 *
	 * @tparam Vout A class inheriting from VectorBase, used to
	 *           contain the results of applying a unary function.
	 *
	 * @tparam Vin A class inheriting from VectorBase, used to
	 *           contain the input values for a unary function.
	 *
	 * @tparam Functor A function object class defining a unary
	 *           function from Vin::value_type to Vout::value_type .
	 */
	template <class Vout, class Vin, typename Functor>
	struct AttributeCopier4Unary
	    : std::binary_function<Vout*, Vin*, void> {
	    /** @brief Copy attributes as required.
	     *
	     * The default behaviour, implemented here, is to copy all
	     * attributes across, along with the S4 object status.
	     *
	     * @param to Non-null pointer to the vector to which
	     *          attributes are to be copied.
	     *
	     * @param from Non-null pointer to the vector from which
	     *          attributes are to be copied.
	     */
	    void operator()(Vout* to, const Vin* from)
	    {
		to->copyAttributes(from, true);
	    }
	};

	/** @brief Check function application for VectorOps::unary().
	 *
	 * An object from an instantiation of this template is used by
	 * by VectorOps::unary() to keep track of any abnormal
	 * conditions arising in the application of the unary
	 * function.  After each individual application of the unary
	 * function, the operator() method of the validator object is
	 * called with the result value and the input value.  The
	 * validator object can then note any abnormalities as part of
	 * its internal state, and possibly modify the result value.
	 *
	 * Once VectorOps::unary() has processed the entire input
	 * vector, it calls the wrapUp() method of the validator
	 * object, which can raise an error or warning.
	 *
	 * The default behaviour, implemented here, is to apply no
	 * validation.  However, this can be overridden by
	 * specializing this template.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::unary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type and \a argument_type appropriately).
	 */
	template <class Functor>
	struct Validator4Unary
	    : std::binary_function<typename Functor::result_type,
				   typename Functor::argument_type, void> {
	    typedef typename Functor::result_type result_type;
	    typedef typename Functor::argument_type argument_type;

	    /** @brief Apply validation to an input-result pair.
	     *
	     * The default behaviour, implemented here, is to apply no
	     * validation.
	     *
	     * @param out Pointer to the result value of the functor
	     *          application to be validated.  It is
	     *          permissible for the function to modify this
	     *          value, and if this happens, the modified value
	     *          will be carried through to the result of the
	     *          VectorOps::unary() invocation.
	     *
	     * @param in Input value to the functor application to be
	     *          validated.
	     */
	    void operator()(result_type* out, const argument_type& in)
	    {}

	    /** @brief Wrap-up processing of a vector.
	     *
	     * This function will be called by VectorOps::unary() once
	     * all the elements of its input vector have been
	     * processed.  The Validator4Unary object can use it to
	     * raise an error or (more typically) a warning in the
	     * light of the validation results.
	     *
	     * The default behaviour, implemented here, is to do
	     * nothing.
	     */
	    void wrapUp()
	    {}
	};

	template <class Vout, class Vin, typename Functor>
	static Vout* unary(const Vin* v, Functor f);
    };  // class VectorOps

    template <class Vout, class Vin, typename Functor>
    Vout* VectorOps::unary(const Vin* v, Functor f)
    {
	typedef typename Vin::value_type Inval;
	typedef typename Vout::value_type Outval;
	size_t vsize = v->size();
	GCStackRoot<Vout> ans(CXXR_NEW(Vout(vsize)));
	Validator4Unary<Functor> validator;
	for (unsigned int i = 0; i < vsize; ++i) {
	    const Inval arg = (*v)[i];
	    if (isNA(arg))
		(*ans)[i] = NA<Outval>();
	    else {
		Outval result = f(arg);
		validator(&result, arg);
		(*ans)[i] = result;
	    }
	}
	validator.wrapUp();
	AttributeCopier4Unary<Vout, Vin, Functor> attrib_copier;
	attrib_copier(ans, v);
	return ans;
    }
}  // namespace CXXR;

#endif  // VECTOROPS_HPP
