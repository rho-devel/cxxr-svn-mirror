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

/** @file RObject.h
 *
 * @brief Class CXXR::RObject and associated C interface functions.
 */

#ifndef ROBJECT_H
#define ROBJECT_H

#include "R_ext/Boolean.h"
#include "CXXR/SEXPTYPE.h"

#ifdef __cplusplus

#include "CXXR/GCNode.hpp"
#include "CXXR/RHandle.hpp"
#include "CXXR/uncxxr.h"

/** @brief Namespace for the CXXR project.
 *
 * CXXR is a project to refactorize the R interpreter into C++.
 */
namespace CXXR {
    class Environment;
    class PairList;
    class Symbol;

    /** @brief Replacement for CR's ::SEXPREC.
     *
     * This class is the rough equivalent within CXXR of the SEXPREC
     * union within CR.  However, all functionality relating to
     * garbage collection has been factored out into the base class
     * GCNode, and as CXXR development proceeds other functionality
     * will be factored out into derived classes (corresponding
     * roughly, but not exactly, to different ::SEXPTYPE values within
     * CR), or outside the RObject hierarchy altogether.
     *
     * Eventually this class may end up simply as the home of R
     * attributes.
     *
     * @note The word 'object' in the name of this class is used in
     * the sense in which the 'blue book' (Becker <em>et al.</em>
     * [1988]) uses the phrase 'data object'.  Roughly speaking,
     * CXXR::RObject is a base class for the sorts of data items whose
     * existence would be reported by the R function
     * <tt>objects()</tt>.  In particular, it does not imply that
     * the object belongs to an R class.
     *
     * @invariant The class currently aims to enforce the following
     * invariants in regard to each RObject:
     * <ul>
     *
     * <li><tt>m_has_class</tt> is true iff the object has the class
     * attribute.</li>
     *
     * <li>Each attribute in the list of attributes must have a Symbol
     * as its tag.  Null tags are not allowed.</li>
     *
     * <li>Each attribute must have a distinct tag: no duplicates
     * allowed.</li>
     *
     * <li>No attribute may have a null value: an attempt to set the
     * value of an attribute to null will result in the removal of the
     * attribute altogether.
     * </ul>
     * The CR code in attrib.cpp applies further consistency
     * conditions on attributes, but these are not yet enforced via
     * the class interface.
     *
     * @par <tt>const RObject*</tt> policy:
     * There is an inherent tension between the way CR is implemented
     * and the 'const-correctness' that C++ programmers seek, and this
     * particularly arises in connection with pointers to objects of
     * classes derived from RObject.  CR accesses such objects
     * exclusively using ::SEXP, which is a non-const pointer.  (The
     * occasional use within the CR code of <tt>const SEXP</tt> is
     * misguided: the compiler interprets this in effect as
     * <tt>RObject* const</tt>, not as <tt>const RObject*</tt>.)  One
     * possible policy would be simply never to use <tt>const T*</tt>,
     * where \c T is \c RObject* or a class inheriting from it: that
     * would remove any need for <tt>const_cast</tt>s at the interface
     * between new CXXR code and code inherited from CR.  But CXXR
     * tries to move closer to C++ idiom than that, notwithstanding
     * the resulting need for <tt>const_cast</tt>s at the interface,
     * and applies a policy driven by the following considerations:
     * <ol>
     *
     * <li>RObject::evaluate() cannot return a <code>const
     * RObject*</code>, because some functions return a pointer to an
     * <code>Environment</code>, which may well need subsequently to
     * be modified e.g. by inserting or changing bindings.</li>
     *
     * <li>This in turn means that RObject::evaluate() cannot itself
     * be a <code>const</code> function, because the default
     * implementation returns <code>this</code>. (Another view would
     * be that the default implementation is an elided copy.)  Also,
     * Promise objects change internally when they are evaluated
     * (though this might conceivably be swept up by
     * <code>mutable</code>).</li>
     *
     * <li>It is a moot point whether FunctionBase::apply() can be
     * <code>const</code>.  Closure::apply() entails evaluating the
     * body, and if the body is regarded as part of the Closure
     * object, that would point to <code>apply()</code> not being
     * <code>const</code>. (Note that some of the types which
     * Rf_mkCLOSXP() accepts as a Closure body use the default
     * RObject::evaluate(), so Point&nbsp;2 definitely applies.)</li>
     *
     * <li>Should PairList objects and suchlike emulate (roughly
     * speaking) (a) <code>list&lt;pair&lt;const RObject*, const
     * RObject*&gt; &gt;</code> (where the first element of the pair
     * is the tag and the second the 'car'),
     * (b) <code>list&lt;pair&lt;const RObject*, RObject*&gt;
     * &gt;</code> or (c) <code>list&lt;pair&lt;RObject*, RObject*&gt;
     * &gt;</code> ? Since the 'cars' of list elements will often need
     * to be evaluated, Point&nbsp;2 rules out (a).  At present CXXR
     * follows (b).</li>
     *
     * <li>Since Symbol objects may well need to be evaluated,
     * Symbol::obtain() returns a non-const pointer; similarly,
     * CachedString::obtain() returns a non-const pointer to a
     * CachedString object.</li>
     * </ol>
     *
     * @todo Incorporate further attribute consistency checks within
     * the class interface.  Possibly make setAttribute() virtual so
     * that these consistency checks can be tailored according to the
     * derived class.
     */
    class RObject : public GCNode {
    public:
	/** @brief Class of function object that does nothing to an RObject.
	 *
	 * This struct is typically used as a default template
	 * parameter, for example in FixedVector.
	 */
	struct DoNothing : std::unary_function<RObject*, void> {
	    void operator()(RObject*)
	    {}
	};

	/** @brief Get object attributes.
	 *
	 * @return Pointer to the attributes of this object.
	 *
	 * @note Callers should beware that derived classes may
	 * override this function with one that gives rise to garbage
	 * collection.
	 */
	virtual const PairList* attributes() const;

	/** @brief Remove all attributes.
	 */
	virtual void clearAttributes();

	/** @brief Return pointer to a copy of this object.
	 *
	 * This function creates a copy of this object, and returns a
	 * pointer to that copy.
	 *
	 * Generally this function (and the copy constructors it
	 * utilises) will attempt to create a 'deep' copy of the
	 * object; this follows standard practice within C++, and it
	 * is intended to extend this practice as CXXR development
	 * continues.
	 *
	 * However, if the pattern object contains unclonable
	 * subobjects, then the created copy will at the relevant
	 * places simply contain pointers to those subobjects, i.e. to
	 * that extent the copy is 'shallow'.  This is managed using
	 * the smart pointers defined by nested class RObject::Handle.
	 *
	 * @return a pointer to a clone of this object, or a null
	 * pointer if this object cannot be cloned.
	 *
	 * @note Derived classes should exploit the covariant return
	 * type facility to return a pointer to the type of object
	 * being cloned.
	 */
	virtual RObject* clone() const
	{
	    return 0;
	}

	/** @brief Return a pointer to a copy of an object.
	 *
	 * @tparam T RObject or a type derived from RObject.
	 *
	 * @param pattern Either a null pointer or a pointer to the
	 *          object to be cloned.
	 *
	 * @return Pointer to a clone of \a pattern, or a null pointer
	 * if \a pattern cannot be cloned or is itself a null pointer.
	 */
	template <class T>
	static T* clone(const T* pattern)
	{
	    return pattern ? static_cast<T*>(pattern->clone()) : 0;
	}

	/** @brief Clone an object if it has at least one owner.
	 *
	 * @tparam T RObject or a type derived from RObject.
	 *
	 * @param pattern Either a null pointer or a pointer to the
	 *          object which may need to be cloned.
	 *
	 * @return If \a pattern is non-null, and points to a clonable
	 * RObject with at least one owner, then a pointer to a clone
	 * of \a pattern.  Otherwise the pointer \a pattern itself,
	 * with const cast away.
	 */
	template <class T>
	static T* cloneIfOwned(const T* pattern)
	{
	    T* cln = 0;
	    if (pattern && pattern->owned())
		cln = clone(pattern);
	    return (cln ? cln : const_cast<T*>(pattern));
	}

	/** @brief Copy an attribute from one RObject to another.
	 *
	 * @param name Non-null pointer to the Symbol naming the
	 *          attribute to be copied.
	 *
	 * @param source Non-null pointer to the object from which
	 *          the attribute are to be copied.  If \a source does
	 *          not have an attribute named \a name , then the
	 *          function has no effect.
	 */
	void copyAttribute(const Symbol* name, const RObject* source)
	{
	    const RObject* att = source->getAttribute(name);
	    if (att)
		setAttribute(name, att);
	}

	/** @brief Copy attributes from one RObject to another.
	 *
	 * Any existing attributes of \a *this are discarded.
	 *
	 * @param source Non-null pointer to the object from which
	 *          attributes are to be copied.
	 *
	 * @param copyS4 If true, the status of \a source as an S4
	 *          object (or not) is also copied to \a *this .
	 */
	void copyAttributes(const RObject* source, bool copyS4);

	/** @brief Evaluate object in a specified Environment.
	 *
	 * @param env Pointer to the environment in which evaluation
	 *          is to take place.
	 *
	 * @return Pointer to the result of evaluation.
	 */
	virtual const RObject* evaluate(Environment* env) const;

	/** @brief Get the value a particular attribute.
	 *
	 * @param name Pointer to a \c Symbol giving the name of the
	 *          sought attribute.
	 *
	 * @return pointer to the value of the attribute with \a name,
	 * or a null pointer if there is no such attribute.
	 *
	 * @note Implementers of derived classes should ensure that
	 * any function overriding this <em>will not</em> give rise to
	 * garbage collection.
	 */
	virtual const RObject* getAttribute(const Symbol* name) const;

	/** @brief Has this object any attributes?
	 *
	 * @return true iff this object has any attributes.
	 *
	 * @note Implementers of derived classes should ensure that
	 * any function overriding this <em>will not</em> give rise to
	 * garbage collection.
	 */
	virtual bool hasAttributes() const
	{
	    return RObject::attributes() != 0;
	}

	/** @brief Has this object the class attribute?
	 *
	 * @return true iff this object has the class attribute.
	 */
	bool hasClass() const
	{
	    return m_type < 0;
	}

	/** @brief Is this an S4 object?
	 *
	 * @return true iff this is an S4 object.
	 */
	bool isS4Object() const
	{
	    return (m_type & s_S4_mask);
	}

	/** @brief Reproduce the \c gp bits field used in CR.
	 *
	 * This function is used to reproduce the
	 * <tt>sxpinfo_struct.gp</tt> field used in CR.  It should be
	 * used exclusively for serialization.  Refer to the 'R
	 * Internals' document for details of this field.
	 *
	 * @return the reconstructed \c gp bits field (within the
	 * least significant 16 bits).
	 *
	 * @note If this function is overridden in a derived class,
	 * the overriding function should call packGPBits() for its
	 * immediate base class, and then 'or' further bits into the
	 * result.
	 */
	virtual unsigned int packGPBits() const;

	/** @brief Set or remove an attribute.
	 *
	 * Sets attribute \a name to a lazy copy of \a value .
	 *
	 * @param name Pointer to the Symbol naming the attribute to
	 *          be set or removed.
	 *
	 * @param value Pointer to the value to be ascribed to the
	 *          attribute, or a null pointer if the attribute is
	 *          to be removed.  The object whose attribute is set
	 *          (i.e. <tt>this</tt>) should be considered to
	 *          assume ownership of \a value, which should
	 *          therefore not be subsequently altered externally.
	 */
	virtual void setAttribute(const Symbol* name, const RObject* value);

	/** @brief Replace the attributes of an object.
	 *
	 * @param new_attributes Pointer to the start of the new list
	 *          of attributes.  May be a null pointer, in which
	 *          case all attributes are removed.  The object whose
	 *          attributes are set (i.e. <tt>this</tt>) should be
	 *          considered to assume ownership of the 'car' values
	 *          in \a new_attributes ; they should therefore not
	 *          be subsequently altered externally.
	 *
	 * @note The \a new_attributes list should conform to the
	 * class invariants.  However, attributes with null values are
	 * silently discarded, and if duplicate attributes are
	 * present, only the last one is heeded (and if the last
	 * setting has a null value, the attribute is removed altogether).
	 */
	void setAttributes(const PairList* new_attributes);

	/** @brief Set the status of this RObject as an S4 object.
	 *
	 * @param on true iff this is to be considered an S4 object.
	 *          CXXR raises an error if an attempt is made to
	 *          unset the S4 object status of an S4Object
	 *          (::S4SXP), whereas CR (at least as of 2.7.2) permits
	 *          this.
	 */
	void setS4Object(bool on);

	/** @brief Get an object's ::SEXPTYPE.
	 *
	 * @return ::SEXPTYPE of this object.
	 */
	SEXPTYPE sexptype() const
	{
	    return SEXPTYPE(m_type & s_sexptype_mask);
	}

	/** @brief Name within R of this type of object.
	 *
	 * @return the name by which this type of object is known
	 *         within R.
	 */
	virtual const char* typeName() const;

	/** @brief Interpret the \c gp bits field used in CR.
	 *
	 * This function is used to interpret the
	 * <tt>sxpinfo_struct.gp</tt> field used in CR in a way
	 * appropriate to a particular node class.  It should be
	 * used exclusively for deserialization.  Refer to the 'R
	 * Internals' document for details of this field.
	 *
	 * @param gpbits the \c gp bits field (within the
	 *          least significant 16 bits).
	 *
	 * @note If this function is overridden in a derived class,
	 * the overriding function should also pass its argument to
	 * unpackGPBits() for its immediate base class.
	 */
	virtual void unpackGPBits(unsigned int gpbits);

	// Virtual functions of GCNode:
	void detachReferents()
	{
	    m_attrib.detach();
	}

	void visitReferents(const_visitor* v) const;
    protected:
	/**
	 * @param stype Required type of the RObject.
	 */
	explicit RObject(SEXPTYPE stype = CXXSXP)
	    : m_type(stype & s_sexptype_mask), m_owners1(1), m_missing(0),
	      m_argused(0), m_active_binding(false), m_binding_locked(false)
	{}

	/** @brief Copy constructor.
	 *
	 * @param pattern Object to be copied.
	 */
	RObject(const RObject& pattern);

	virtual ~RObject() {}
    private:
	static const unsigned char s_sexptype_mask = 0x3f;
	static const unsigned char s_S4_mask = 0x40;
	static const unsigned char s_class_mask = 0x80;
	static const unsigned char s_inc_owners[8];  // Look-up table
	  // for incrementing m_owners1.
	static const unsigned char s_dec_owners[8];  // Look-up table
	  // for decrementing m_owners1.

	signed char m_type;  // The least-significant six bits hold
	  // the SEXPTYPE.  The sign bit is set if the object has a
	  // class attribute.  Bit 6 is set to denote an S4 object.
	mutable unsigned char m_owners1;  // Number of RHandles claiming
	  // ownership of this RObject, plus 1.  A value of zero
	  // signifies that this object is to be regarded as its own
	  // clone, so duplication that would otherwise take place can
	  // be skipped: this applies in particular to Environments,
	  // Symbols and CachedStrings.
    public:
	// The following field is used only in connection with objects
	// inheriting from class ConsCell (and fairly rarely then), so
	// it would more logically be placed in that class (and
	// formerly was within CXXR).  It is placed here so that the
	// ubiquitous PairList objects can be squeezed into 32 bytes
	// (on 32-bit architecture), for improved cache efficiency.
	// This field is obsolescent in any case, and should be got
	// rid of entirely in due course:

	// 'Scratchpad' field used in handling argument lists,
	// formerly hosted in the 'gp' field of sxpinfo_struct.
	unsigned m_missing     : 2;
	
	// Similarly the following three obsolescent fields squeezed
	// in here are used only in connection with objects of class
	// PairList (and only rarely then), so they would more
	// logically be placed in that class (and formerly were within
	// CXXR).
	
	// 'Scratchpad' field used in handling argument lists,
	// formerly hosted in the 'gp' field of sxpinfo_struct.
	unsigned m_argused    : 2;

	// Used when the contents of an Environment are represented as
	// a PairList, for example during serialization and
	// deserialization, and formerly hosted in the gp field of
	// sxpinfo_struct.
	bool m_active_binding : 1;
	bool m_binding_locked : 1;
    private:
	friend class RHandleBase;
	friend class RPointerBase;

	RHandle<PairList> m_attrib;

	void decOwners() const
	{
	    m_owners1 = s_dec_owners[m_owners1];
	}

	void incOwners() const
	{
	    m_owners1 = s_inc_owners[m_owners1];
	}

	bool owned() const
	{
	    return m_owners1 >= 2;
	}

	bool shared() const
	{
	    return m_owners1 >= 3;
	}
    };

    // ***** Inlined methods of RHandleBase:
    inline RHandleBase::RHandleBase(const RObject* target)
	: GCEdgeBase(target)
    {
	if (target)
	    target->incOwners();
#ifndef LAZYCOPY
	if (sharing()) {
	    GCNode::GCInhibitor inh;
	    RObject* clone = asset()->clone();
	    if (clone) {
		RHandleBase tmp(clone);
		swap(tmp);
	    }
	}
#endif
    }

    inline RHandleBase::~RHandleBase()
    {
	const RObject* tgt = asset();
	if (tgt)
	    tgt->decOwners();
    }

    inline const RObject* RHandleBase::asset() const
    {
	return static_cast<const RObject*>(target());
    }

    inline bool RHandleBase::sharing() const
    {
	const RObject* tgt = asset();
	return (tgt && tgt->shared());
    }
}  // namespace CXXR

/** @brief Pointer to an RObject.
 *
 * In CR, almost all interpreter code could access R objects only \e
 * via the opaque pointer SEXP.  In CXXR, C code continues to see SEXP
 * as an opaque pointer, but C++ code sees SEXP defined as 'pointer to
 * RObject'.
 *
 * @note This typedef is provided for compatibility with code
 * inherited from CR.  New CXXR code should write RObject*
 * explicitly.
 */
typedef CXXR::RObject *SEXP;

extern "C" {
#else /* if not __cplusplus */
    // Opaque pointer (SEXPREC doesn't exist in CXXR):
    typedef struct SEXPREC *SEXP;

#endif /* __cplusplus */

    /** @brief Get the attributes of a CXXR::RObject.
     *
     * @param x Pointer to the CXXR::RObject whose attributes are required.
     *
     * @return Pointer to the attributes object of \a x , or 0 if \a x is
     * a null pointer.
     */
    SEXP ATTRIB(SEXP x);

    /** @brief Replace the attributes of \a to by those of \a from.
     *
     * The status of \a to as an S4 Object is also copied from \a from .
     * 
     * @param to Pointer to CXXR::RObject.
     *
     * @param from Pointer to another CXXR::RObject.
     */
    void DUPLICATE_ATTRIB(SEXP to, SEXP from);

    /** @brief Is this an S4 object?
     *
     * @param x Pointer to CXXR::RObject.
     *
     * @return true iff \a x is an S4 object.  Returns false if \a x
     * is 0.
     */
#ifndef __cplusplus
    Rboolean IS_S4_OBJECT(SEXP x);
#else
    inline Rboolean IS_S4_OBJECT(SEXP x)
    {
	return Rboolean(x && x->isS4Object());
    }
#endif

    /** @brief (For use only in serialization.)
     */
#ifdef __cplusplus
    inline int LEVELS(SEXP x) {return x->packGPBits();}
#endif

    /** @brief Obsolete in CXXR.
     */
#define NAMED(x) 0

    /** @brief Does an object have a class attribute?
     *
     * @param x Pointer to a CXXR::RObject.
     *
     * @return true iff \a x has a class attribute.  Returns false if \a x
     * is 0.
     */
#ifndef __cplusplus
    Rboolean OBJECT(SEXP x);
#else
    inline Rboolean OBJECT(SEXP x)
    {
	return Rboolean(x && x->hasClass());
    }
#endif

    /** @brief (For use only in deserialization.)
     */
#ifdef __cplusplus
    inline int SETLEVELS(SEXP x, int v)
    {
	x->unpackGPBits(v);
	return v;
    }
#endif

    /** @brief Replace an object's attributes.
     *
     * @param x Pointer to a CXXR::RObject.
     *
     * @param v Pointer to a PairList giving the new attributes of \a
     *          x.  \a x should be considered to assume ownership of
     *          the 'car' values in \a v ; they should therefore not
     *          be subsequently altered externally.
     *
     * @note Unlike CR, \a v isn't simply plugged into the attributes
     * field of \a x : refer to the documentation for \c
     * RObject::setAttributes() .  In particular, do not attempt to
     * modify the attributes by changing \a v \e after SET_ATTRIB
     * has been called.
     *
     * @note For compatibility with CR, garbage collection is
     * inhibited within this function.
     */
    void SET_ATTRIB(SEXP x, SEXP v);

    /** @brief Obsolete in CXXR.
     */
#define SET_NAMED(a, b)

    /**
     * @deprecated Ought to be private.
     */
#ifndef __cplusplus
    void SET_S4_OBJECT(SEXP x);
#else
    inline void SET_S4_OBJECT(SEXP x)  {x->setS4Object(true);}
#endif

    /** @brief Get object's ::SEXPTYPE.
     *
     * @param x Pointer to CXXR::RObject.
     *
     * @return ::SEXPTYPE of \a x, or ::NILSXP if x is a null pointer.
     */
#ifndef __cplusplus
    SEXPTYPE TYPEOF(SEXP x);
#else
    inline SEXPTYPE TYPEOF(SEXP x)  {return x ? x->sexptype() : NILSXP;}
#endif

    /**
     * @deprecated Ought to be private.
     */
#ifndef __cplusplus
    void UNSET_S4_OBJECT(SEXP x);
#else
    inline void UNSET_S4_OBJECT(SEXP x)  {x->setS4Object(false);}
#endif

    /** @brief Copy attributes, with some exceptions.
     *
     * This is called in the case of binary operations to copy most
     * attributes from one of the input arguments to the output.
     * Note that the Dim, Dimnames and Names attributes are not
     * copied: these should have been assigned elsewhere.  The
     * function also copies the S4 object status.
     *
     * @param inp Pointer to the CXXR::RObject from which attributes are to
     *          be copied.
     *
     * @param ans Pointer to the CXXR::RObject to which attributes are to be
     *          copied.
     *
     * @note The above documentation is probably incomplete: refer to the
     *       source code for further details.
     */
    void Rf_copyMostAttrib(SEXP inp, SEXP ans);

    /** @brief Access a named attribute.
     *
     * @param vec Pointer to the CXXR::RObject whose attributes are to be
     *          accessed.
     *
     * @param name Either a pointer to the symbol representing the
     *          required attribute, or a pointer to a CXXR::StringVector
     *          containing the required symbol name as element 0; in
     *          the latter case, as a side effect, the corresponding
     *          symbol is installed if necessary.
     *
     * @return Pointer to the requested attribute, or a null pointer
     *         if there is no such attribute.
     *
     * @note The above documentation is incomplete: refer to the
     *       source code for further details.
     */
    SEXP Rf_getAttrib(SEXP vec, SEXP name);

    /** @brief Is this the null object pointer?
     *
     * @param s Pointer to a CXXR::RObject.
     *
     * @return TRUE iff the CXXR::RObject pointed to by \a s is either a null
     * pointer (i.e. <tt>== R_NilValue</tt> in CXXR), or is a CXXR::RObject
     * with ::SEXPTYPE ::NILSXP (should not happen in CXXR).
     */
#ifndef __cplusplus
    Rboolean Rf_isNull(SEXP s);
#else
    inline Rboolean Rf_isNull(SEXP s)
    {
	return Rboolean(!s || TYPEOF(s) == NILSXP);
    }
#endif

    /** @brief Does an object have a class attribute?
     *
     * @param s Pointer to a CXXR::RObject.
     *
     * @return TRUE iff the CXXR::RObject pointed to by \a s has a
     * class attribute.
     */
#ifndef __cplusplus
    Rboolean Rf_isObject(SEXP s);
#else
    inline Rboolean Rf_isObject(SEXP s)
    {
	return OBJECT(s);
    }
#endif

    /** @brief Set or remove a named attribute.
     *
     * @param vec Pointer to the CXXR::RObject whose attributes are to be
     *          modified.
     *
     * @param name Either a pointer to the symbol representing the
     *          required attribute, or a pointer to a CXXR::StringVector
     *          containing the required symbol name as element 0; in
     *          the latter case, as a side effect, the corresponding
     *          symbol is installed if necessary.
     *
     * @param val Either the value to which the attribute is to be
     *          set, or a null pointer.  In the latter case the
     *          attribute (if present) is removed.
     *
     * @return Refer to source code.  (Sometimes \a vec, sometimes \a
     * val, sometime a null pointer ...)
     *
     * @note The above documentation is probably incomplete: refer to the
     *       source code for further details.
     */
    SEXP Rf_setAttrib(SEXP vec, SEXP name, SEXP val);

    /** @brief Name of type within R.
     *
     * Translate a ::SEXPTYPE to the name by which it is known within R.
     *
     * @param st The ::SEXPTYPE whose name is required.
     *
     * @return The ::SEXPTYPE's name within R.
     */
    const char* Rf_type2char(SEXPTYPE st);

#ifdef __cplusplus
}
#endif

#endif /* ROBJECT_H */
