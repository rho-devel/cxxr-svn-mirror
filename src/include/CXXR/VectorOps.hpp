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
	/** @brief Control attribute copying for unary functions.
	 *
	 * VectorOps::UnaryFunction takes as a template parameter an
	 * \a AttributeCopier class which determines which attributes
	 * are copied from the input vector to the output vector.
	 *
	 * This class is the default value of the \a AttributeCopier
	 * parameter, and its behaviour is to copy all attributes
	 * across, along with the S4 object status.  If different
	 * behaviour is required, another class can be created using
	 * this class as a model.
	 */
	struct DefaultAttributeCopier4Unary
	    : std::binary_function<RObject*, RObject*, void> {
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
	    void operator()(RObject* to, const RObject* from)
	    {
		to->copyAttributes(from, true);
	    }
	};

	/** @brief Monitor function application for unary functions.
	 *
	 * VectorOps::UnaryFunction takes as a template parameter a \a
	 * FunctorWrapper class.  The apply() method of UnaryFunction
	 * creates an object of the \a FunctorWrapper class, and
	 * delegates to it the task of calling the unary function; the
	 * \a FunctorWrapper object can then monitor any abnormal
	 * conditions that occur, and take appropriate action either
	 * immediately (typically by raising an error) or when
	 * processing of the input vector is complete (typically by
	 * providing one or more warnings).
	 *
	 * This class is the default value of the \a FunctorWrapper
	 * template parameter, and its behaviour is to apply no
	 * monitoring at all.  If different behaviour is required,
	 * another class can be created using this class as a model.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::unary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type and \a argument_type appropriately).
	 */
	template <class Functor>
	class DefaultUnaryFunctorWrapper
	    : public std::unary_function<typename Functor::argument_type,
					 typename Functor::result_type> {
	public:
	    // See para. 3 of ISO14882:2003 Sec. 14.6.2 for why these
	    // typedefs aren't inherited from std::unary_function:
	    typedef typename Functor::argument_type argument_type;
	    typedef typename Functor::result_type result_type;

	    /** @brief Constructor.
	     *
	     * @param f Pointer to an object of type \a Functor
	     *          defining the unary function whose operation
	     *          this \a FunctorWrapper is to monitor.
	     */
	    DefaultUnaryFunctorWrapper(const Functor& f)
		: m_func(f)
	    {}
		
	    /** @brief Monitored invocation of \a f .
	     *
	     * The apply() method of an object instantiating the
	     * VectorOps::UnaryFunction template will call this
	     * function to generate a value for the output vector from
	     * a value from the input vector using the functor \a f .
	     * This function will monitor the operation of \a f , and
	     * take appropriate action if abnormalities occur, for
	     * example by raising an error, modifying the return
	     * value, and/or recording the abnormality for later
	     * reporting by warnings().
	     *
	     * @param in Input value to which \a f is to be applied.
	     *
	     * @result The result of applying \a f to \a in , possibly
	     * modified if abnormalities occurred.
	     */
	    result_type operator()(const argument_type& in)
	    {
		return (m_func)(in);
	    }

	    /** @brief Raise warnings after processing a vector.
	     *
	     * The apply() method of an object instantiating the
	     * VectorOps::UnaryFunction template will call this
	     * function once all the elements of the input vector have
	     * been processed.  Typically this function will do
	     * nothing if no abnormalities have occurred during the
	     * lifetime of this \a FunctorWrapper object , otherwise
	     * it will raise one or more warnings.  (Note that the
	     * lifetime of a \a FunctorWrapper object corresponds to
	     * the processing of an input vector by the apply() method
	     * of UnaryFunction.)
	     *
	     * The default behaviour, implemented here, is to do
	     * nothing.
	     */
	    void warnings()
	    {}
	private:
	    Functor m_func;
	};

	/** @brief Class used to transform a vector elementwise using
	 *         unary function.
	 *
	 * An object of this class is used to map one vector into
	 * new vector of equal size, with each element of the output
	 * vector being obtained from the corresponding element of the
	 * input vector by the application of a unary function.  NAs
	 * in the input vector are carried across into NAs in the
	 * output vector.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::unary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type and \a argument_type appropriately).
	 *
	 * @tparam FunctorWrapper Each invocation of apply() will
	 *           create an object of this class, and delegate to
	 *           it the task of calling the unary function; the \a
	 *           FunctorWrapper objects can then monitor any
	 *           abnormal conditions.  See the description of
	 *           VectorOps::DefaultUnaryFunctionWrapper for
	 *           further information.
	 *
	 * @tparam AttributeCopier The apply() method will create an
	 *           object of this class and use it to determine
	 *           which attributes are copied from the input vector
	 *           to the output vector.  See the description of
	 *           VectorOps::DefaultAttributeCopier4Unary for
	 *           further information.
	 */
	template <typename Functor,
		  class FunctorWrapper = DefaultUnaryFunctorWrapper<Functor>,
		  class AttributeCopier = DefaultAttributeCopier4Unary>
	class UnaryFunction {
	public:
	    /** @brief Constructor.
	     *
	     * @param f Pointer to an object of type \a Functor
	     *          defining the unary function that this
	     *          UnaryFunction object will use to generate an
	     *          output vector from an input vector.
	     */
	    UnaryFunction(const Functor& f)
		: m_f(f)
	    {}

	    /** @brief Apply a unary function to a vector.
	     *
	     * @tparam Vout Class of vector to be produced as a
	     *           result.  It must be possible to assign values
	     *           of the return type of \a f to the elements of
	     *           a vector of type \a Vout .
	     *
	     * @tparam Vin Class of vector to be taken as input.  It
	     *           must be possible implicitly to convert the
	     *           elements of a \a Vin to the input type of \a
	     *           f .
	     *
	     * @param v Non-null pointer to the input vector.
	     *
	     * @result Pointer to the result vector, a newly created
	     * vector of the same size as \a v , with each element of
	     * the output vector being obtained from the corresponding
	     * element of the input vector by the application of \a f
	     * , except that NAs in the input vector are carried
	     * across into NAs in the output vector.
	     */
	    template <class Vout, class Vin>
	    Vout* apply(const Vin* v);
	private:
	    Functor m_f;
	};

	/** @brief Apply a unary function to a vector (simple case).
	 *
	 * This function represents a common special case.  For
	 * customised behaviour, use class UnaryFunction directly.
	 *
	 * @tparam Vout Class of vector to be produced as a result.
	 *           It must be possible to assign values of the
	 *           return type of \a f to the elements of a vector
	 *           of type \a Vout .
	 *
	 * @tparam Vin Class of vector to be taken as input.  It must
	 *           be possible implicitly to convert the elements of
	 *           a \a Vin to the input type of \a f .
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::unary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type and \a argument_type appropriately).
	 *
	 * @param v Non-null pointer to the input vector.
	 *
	 * @result Pointer to the result vector, a newly created
	 * vector of the same size as \a v , with each element of the
	 * output vector being obtained from the corresponding element
	 * of the input vector by the application of \a f , except
	 * that NAs in the input vector are carried across into NAs in
	 * the output vector.  All attributes of \a v are copied to
	 * the result; the status of \a v as an S4 object (or not) is
	 * also copied across.
	 */
	template <class Vout, class Vin, typename Functor>
	static Vout* unary(const Vin* v, Functor f)
	{
	    UnaryFunction<Functor> uf(f);
	    // Refer to para. 4 of ISO14882:2003 sec 14.2 for the need
	    // for the following syntax:
	    return uf.template apply<Vout>(v);
	}
    private:
	// Not implemented.  Declared private to prevent VectorOps
	// objects being created.
	VectorOps();
    };  // class VectorOps

    template <typename Functor, class FunctorWrapper, class AttributeCopier>
    template <class Vout, class Vin>
    Vout* VectorOps::UnaryFunction<Functor,
				   FunctorWrapper,
				   AttributeCopier>::apply(const Vin* v)
    {
	typedef typename Vin::value_type Inval;
	typedef typename Vout::value_type Outval;
	size_t vsize = v->size();
	GCStackRoot<Vout> ans(CXXR_NEW(Vout(vsize)));
	FunctorWrapper fwrapper(m_f);
	for (size_t i = 0; i < vsize; ++i) {
	    const Inval arg = (*v)[i];
	    Outval result;
	    if (isNA(arg))
		result = NA<Outval>();
	    else
		result = fwrapper(arg);
	    (*ans)[i] = result;
	    // (*ans)[i] = (isNA(arg) ? NA<Outval>() : fwrapper(arg));
	}
	fwrapper.warnings();
	AttributeCopier attrib_copier;
	attrib_copier(ans, v);
	return ans;
    }
    
}  // namespace CXXR;

#endif  // VECTOROPS_HPP
