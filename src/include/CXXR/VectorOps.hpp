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

#include "CXXR/VectorBase.h"
#include "CXXR/errors.h"

namespace CXXR {
   /** @brief Services to support common operations on R vectors and arrays.
     *
     * This class, all of whose members are static, encapsulates
     * services supporting various commonly occurring operations on R
     * vector objects, including R matrices and arrays.
     */
    namespace VectorOps {

	// ***** UNARY FUNCTIONS *****

	/** @brief Control attribute copying for unary functions.
	 *
	 * VectorOps::UnaryFunction takes as a template parameter an
	 * \a AttributeCopier class which determines which attributes
	 * are copied from the input vector to the output vector.
	 *
	 * This class is a possible value of the \a AttributeCopier
	 * parameter, and its behaviour is to copy all attributes
	 * across, along with the S4 object status.
	 */
	struct CopyAllAttributes
	    : std::binary_function<VectorBase*, VectorBase*, void> {
	    /** @brief Copy all attributes and S4 object status.
	     *
	     * @param to Non-null pointer to the vector to which
	     *          attributes are to be copied.
	     *
	     * @param from Non-null pointer to the vector from which
	     *          attributes are to be copied.
	     */
	    void operator()(VectorBase* to, const VectorBase* from)
	    {
		to->copyAttributes(from, true);
	    }
	};

	/** @brief Control attribute copying for unary functions.
	 *
	 * VectorOps::UnaryFunction takes as a template parameter an
	 * \a AttributeCopier class which determines which attributes
	 * are copied from the input vector to the output vector.
	 *
	 * This class can be used as the value of the \a AttributeCopier
	 * parameter, and its behaviour is to copy no attributes at all.
	 */
	struct CopyNoAttributes
	    : std::binary_function<VectorBase*, VectorBase*, void> {
	    /** @brief Copy no attributes.
	     */
	    void operator()(VectorBase*, const VectorBase*)
	    {}
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
	class NullUnaryFunctorWrapper
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
	    NullUnaryFunctorWrapper(const Functor& f)
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
	 * @tparam AttributeCopier The apply() method will create an
	 *           object of this class and use it to determine
	 *           which attributes are copied from the input vector
	 *           to the output vector.  See the description of
	 *           VectorOps::CopyAllAttributes for an example.
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
	 *           VectorOps::NullUnaryFunctionWrapper for
	 *           further information.
	 */
	template <class AttributeCopier, typename Functor,
		  class FunctorWrapper = NullUnaryFunctorWrapper<Functor> >
	class UnaryFunction {
	public:
	    /** @brief Constructor.
	     *
	     * @param f Pointer to an object of type \a Functor
	     *          defining the unary function that this
	     *          UnaryFunction object will use to generate an
	     *          output vector from an input vector.
	     */
	    UnaryFunction(const Functor& f = Functor())
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

	/** @brief Create a UnaryFunction object from a functor.
	 *
	 * @tparam AttributeCopier The apply() method will create an
	 *           object of this class and use it to determine
	 *           which attributes are copied from the input vector
	 *           to the output vector.  See the description of
	 *           VectorOps::CopyAllAttributes for an example.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::unary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type and \a argument_type appropriately).
	 *
	 * @param f Pointer to an object of type \a Functor defining
	 *          the unary function that this UnaryFunction object
	 *          will use to generate an output vector from an
	 *          input vector.
	 */
	template <class AttributeCopier, typename Functor>
	static UnaryFunction<AttributeCopier, Functor>
	makeUnaryFunction(Functor f)
	{
	    return UnaryFunction<AttributeCopier, Functor>(f);
	}

	// ***** BINARY FUNCTIONS *****

	/** @brief Are binary operands consistent?
	 *
	 * This function checks the operands of a binary vector
	 * function to verify that they are compatible with each
	 * other, and raises an error if not.
	 *
	 * An error will be raised in the following circumstances:
	 * <ul>
	 * <li>Both operands are arrays, but with different
	 * dimensions.</li>
	 *
	 * <li>Both operands are time series, but they are not
	 * conformable (i.e. they do not have the same start time, end
	 * time and frequency).</li>
	 *
	 * <li>Just one operand is a time series, and the
	 * non-time-series operand is longer than the time-series
	 * operand.</li>
	 * </ul>
	 *
	 * @param vl Non-null pointer to the first operand.
	 *
	 * @param vr Non-null pointer to the second operand.
	 */
	void checkOperandsConformable(const VectorBase* vl,
				      const VectorBase* vr);

	/** @brief Control attribute copying for binary functions.
	 *
	 * VectorOps::BinaryFunction takes as a template parameter an
	 * \a AttributeCopier class which determines which attributes
	 * are copied from the input vectors to the output
	 * vector. This class can be used as the value of the \a
	 * AttributeCopier parameter, and acts as follows:
	 * <ul>
	 * <li>If both operands are arrays (in which case they must
	 * have the same dimensions), the result will be an array with
	 * the same dimensions.  Dimension names are taken from the
	 * first operand if it has them, otherwise from the second
	 * operand, if it has them.</li>
	 *
	 * <li>If just one operand is array, the result will be an
	 * array with the same dimensions.  Dimension names are taken
	 * from the array operand, if it has them.</li>
	 *
	 * <li>If neither operand is an array, the 'names' attribute
	 * is taken from the first operand if it has them, otherwise
	 * from the second operand, if it has them.</li>
	 *
	 * <li>If both operands are time series (in which case they
	 * must have the same parameters, i.e. start time, end time
	 * and frequency), the result will be a time series with the
	 * same parameters.  The result will take its class attribute
	 * from the first operand if it has one, otherwise from the
	 * second operand, if it has one.</li>
	 *
	 * <li>If just one operand is a time series, the result will
	 * be a time series with the same parameters.  The result from
	 * take its class attribute from the time-series operand, if
	 * it has one.</li>
	 * </ul>
	 *
	 * Note that no class attribute is applied to the result
	 * unless at least one of the operands is a time series.
	 */
	class GeneralBinaryAttributeCopier {
	public:
	    /** @brief Copy attributes as described above.
	     *
	     * @param vout Non-null pointer to the vector to which
	     *          attributes are to be copied.
	     *
	     * @param vl Non-null pointer to the first operand.
	     *
	     * @param vr Non-null pointer to the second operand.
	     */
	    void operator()(VectorBase* vout,
			    const VectorBase* vl, const VectorBase* vr)
	    {
		if (!vl->attributes() && !vr->attributes())
		    return;
		apply(vout, vl, vr);
	    }
	private:
	    // Deal with non-trivial cases:
	    void apply(VectorBase* vout,
		    const VectorBase* vl, const VectorBase* vr);
	};

	/** @brief Monitor function application for binary functions.
	 *
	 * VectorOps::BinaryFunction takes as a template parameter a \a
	 * FunctorWrapper class.  The apply() method of BinaryFunction
	 * creates an object of the \a FunctorWrapper class, and
	 * delegates to it the task of calling the binary function; the
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
	 *           instantiation of the std::binary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type , \a first_argument_type and \a
	 *           second_argument_type appropriately).
	 */
	template <class Functor>
	class NullBinaryFunctorWrapper
	    : public std::binary_function<typename Functor::first_argument_type,
					  typename Functor::second_argument_type,
					  typename Functor::result_type> {
	public:
	    // See para. 3 of ISO14882:2003 Sec. 14.6.2 for why these
	    // typedefs aren't inherited from std::unary_function:
	    typedef typename Functor::first_argument_type first_argument_type;
	    typedef typename Functor::second_argument_type second_argument_type;
	    typedef typename Functor::result_type result_type;

	    /** @brief Constructor.
	     *
	     * @param f Pointer to an object of type \a Functor
	     *          defining the binary function whose operation
	     *          this \a FunctorWrapper is to monitor.
	     */
	    NullBinaryFunctorWrapper(const Functor& f)
		: m_func(f)
	    {}
		
	    /** @brief Monitored invocation of \a f .
	     *
	     * The apply() method of an object instantiating the
	     * VectorOps::BinaryFunction template will call this
	     * function to generate a value for an element of the
	     * output vector from the values of the corresponding
	     * elements of the operands, using the functor \a f .
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
	    result_type operator()(const first_argument_type& left,
				   const second_argument_type& right)
	    {
		return (m_func)(left, right);
	    }

	    /** @brief Raise warnings after processing a vector.
	     *
	     * The apply() method of an object instantiating the
	     * VectorOps::BinaryFunction template will call this
	     * function once all the elements of the input vector have
	     * been processed.  Typically this function will do
	     * nothing if no abnormalities have occurred during the
	     * lifetime of this \a FunctorWrapper object , otherwise
	     * it will raise one or more warnings.  (Note that the
	     * lifetime of a \a FunctorWrapper object corresponds to
	     * the processing of an input vector by the apply() method
	     * of BinaryFunction.)
	     *
	     * The default behaviour, implemented here, is to do
	     * nothing.
	     */
	    void warnings()
	    {}
	private:
	    Functor m_func;
	};

	/** @brief Class used to apply a binary function to vectors.
	 *
	 * An object of this class is used to create a vector from two
	 * others (the operands), which must be conformable as tested
	 * by checkOperandsConformable().
	 * 
	 * The result will have the same number of elements as the
	 * larger of the operands, and each element of the result is
	 * obtained by applying the binary function to the
	 * corresponding elements of the two operands (treating them
	 * for this purpose as vectors).  If the operands are of
	 * unequal length, the elements in the shorter operand are
	 * recycled as necessary; in this case a warning is raised if
	 * shorter operand has non-zero length but its length is not
	 * an exact submultiple of the length of the longer operand.
	 * If either of the operand elements is NA then the result
	 * element will automatically be set to NA without invoking
	 * the binary function.
	 *
	 * @tparam AttributeCopier The apply() method will create an
	 *           object of this class and use it to determine
	 *           which attributes are copied from the operands
	 *           to the output vector.  See the description of
	 *           VectorOps::GeneralBinaryAttributeCopier for a
	 *           commonly used example.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::binary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type , \a first_argument_type and \a
	 *           second_argument_type appropriately).
	 *
	 * @tparam FunctorWrapper Each invocation of apply() will
	 *           create an object of this class, and delegate to
	 *           it the task of calling the binary function; the \a
	 *           FunctorWrapper objects can then monitor any
	 *           abnormal conditions.  See the description of
	 *           VectorOps::NullBinaryFunctionWrapper for
	 *           further information.
	 */
	template <class AttributeCopier, typename Functor,
		  class FunctorWrapper = NullBinaryFunctorWrapper<Functor> >
	class BinaryFunction {
	public:
	    /** @brief Constructor.
	     *
	     * @param f Pointer to an object of type \a Functor
	     *          defining the binary function that this
	     *          BinaryFunction object will use to generate an
	     *          output vector from the operands.
	     */
	    BinaryFunction(const Functor& f = Functor())
		: m_f(f)
	    {}

	    /** @brief Apply a binary function to a vector.
	     *
	     * @tparam Vout Class of vector to be produced as a
	     *           result.  It must be possible to assign values
	     *           of the return type of \a f to the elements of
	     *           a vector of type \a Vout .
	     *
	     * @tparam Vl Class of vector forming the first operand.  It
	     *           must be possible implicitly to convert the
	     *           elements of a \a Vl to the type of the first
	     *           argument of \a f .
	     *
	     * @tparam Vr Class of vector forming the second operand.  It
	     *           must be possible implicitly to convert the
	     *           elements of a \a Vr to the type of the second
	     *           argument of \a f .
	     *
	     * @param vl Non-null pointer to the first operand.
	     *
	     * @param vr Non-null pointer to the second operand.
	     *
	     * @result Pointer to the result vector.
	     */
	    template <class Vout, class Vl, class Vr>
	    Vout* apply(const Vl* vl, const Vr* vr);
	private:
	    Functor m_f;

	    // This is called by apply() to calculate the elements of
	    // the result, with 'flag' set as follows: -1 if the first
	    // operand is shorter, 0 if the operands are of equal
	    // length, or +1 if the second operand is shorter.
	    template <int flag, class Vout, class Vl, class Vr>
	    void mapElements(Vout* vout, const Vl* vl, const Vr* vr);
	};

	/** @brief Create a BinaryFunction object from a functor.
	 *
	 * @tparam AttributeCopier The apply() method will create an
	 *           object of this class and use it to determine
	 *           which attributes are copied from the input vector
	 *           to the output vector.  See the description of
	 *           VectorOps::GeneralBinaryAttributeCopier for a
	 *           commonly used example.
	 *
	 * @tparam Functor a function object class inheriting from an
	 *           instantiation of the std::binary_function template
	 *           (or otherwise defining nested types \a
	 *           result_type , \a first_argument_type and \a
	 *           second_argument_type appropriately).
	 *
	 * @param f Pointer to an object of type \a Functor defining
	 *          the binary function that the resulting BinaryFunction object
	 *          will use to generate an output vector from the operands.
	 */
	template <class AttributeCopier, typename Functor>
	static BinaryFunction<AttributeCopier, Functor>
	makeBinaryFunction(Functor f)
	{
	    return BinaryFunction<AttributeCopier, Functor>(f);
	}

	// ***** Implementations of non-inlined templated functions. *****

	template <class AttributeCopier, typename Functor, class FunctorWrapper>
	template <class Vout, class Vin>
	Vout* UnaryFunction<AttributeCopier,
			    Functor,
			    FunctorWrapper>::apply(const Vin* v)
	{
	    typedef typename Vin::element_type Inelt;
	    typedef typename Vout::element_type Outelt;
	    size_t vsize = v->size();
	    GCStackRoot<Vout> ans(CXXR_NEW(Vout(vsize)));
	    FunctorWrapper fwrapper(m_f);
	    for (size_t i = 0; i < vsize; ++i) {
		const Inelt elt = (*v)[i];
		Outelt result;
		if (isNA(elt))
		    result = ElementTraits<Outelt>::NA();
		else
		    result = fwrapper(elementData(elt));
		(*ans)[i] = result;
	    }
	    fwrapper.warnings();
	    AttributeCopier attrib_copier;
	    attrib_copier(ans, v);
	    return ans;
	}

	template <class AttributeCopier, typename Functor, class FunctorWrapper>
	template <class Vout, class Vl, class Vr>
	Vout* BinaryFunction<AttributeCopier,
			     Functor,
			     FunctorWrapper>::apply(const Vl* vl, const Vr* vr)
	{
	    checkOperandsConformable(vl, vr);
	    size_t lsize = vl->size();
	    size_t rsize = vr->size();
	    GCStackRoot<Vout> ans;
	    if (lsize == rsize) {
		ans = CXXR_NEW(Vout(lsize));
		mapElements<0>(ans.get(), vl, vr);
	    } else if (lsize > rsize) {
		ans = CXXR_NEW(Vout(lsize));
		mapElements<1>(ans.get(), vl, vr);
	    } else {
		ans = CXXR_NEW(Vout(rsize));
		mapElements<-1>(ans.get(), vl, vr);
	    }
	    AttributeCopier attrib_copier;
	    attrib_copier(ans, vl, vr);
	    return ans;
	}

	template <class AttributeCopier, typename Functor, class FunctorWrapper>
	template <int flag, class Vout, class Vl, class Vr>
	void BinaryFunction<AttributeCopier,
			    Functor,
			    FunctorWrapper>::mapElements(Vout* vout,
							 const Vl* vl,
							 const Vr* vr)
	{
	    typedef typename Vl::element_type Lelt;
	    typedef typename Vr::element_type Relt;
	    typedef typename Vout::element_type Outelt;
	    size_t lsize = vl->size();
	    size_t rsize = vr->size();
	    if ((flag < 0 && rsize%lsize != 0)
		|| (flag > 0 && lsize%rsize != 0))
		Rf_warning(_("longer object length is not"
			     " a multiple of shorter object length"));
	    size_t outsize = vout->size();
	    FunctorWrapper fwrapper(m_f);
	    size_t il = 0;
	    size_t ir = 0;
	    size_t iout = 0;
	    while (iout < outsize) {
		const Lelt lelt = (*vl)[il++];
		const Relt relt = (*vr)[ir++];
		Outelt result;
		if (isNA(lelt) || isNA(relt))
		    result = ElementTraits<Outelt>::NA();
		else
		    result = fwrapper(elementData(lelt), elementData(relt));
		(*vout)[iout++] = result;
		if (flag < 0 && il == lsize)
		    il = 0;
		if (flag > 0 && ir == rsize)
		    ir = 0;
	    }
	    fwrapper.warnings();
	}
    }  // namespace VectorOps
}  // namespace CXXR

#endif  // VECTOROPS_HPP
