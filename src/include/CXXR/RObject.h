/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008 Andrew R. Runnalls, subject to such other
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

#ifdef __cplusplus

#include "CXXR/FlagWord.hpp"
#include "CXXR/GCNode.hpp"

extern "C" {
#endif

    /* type for length of vectors etc */
    typedef int R_len_t; /* will be long later, LONG64 or ssize_t on Win64 */
#define R_LEN_T_MAX INT_MAX

    /* Fundamental Data Types:  These are largely Lisp
     * influenced structures, with the exception of LGLSXP,
     * INTSXP, REALSXP, CPLXSXP and STRSXP which are the
     * element types for S-like data objects.

     * Note that the gap of 11 and 12 below is because of
     * the withdrawal of native "factor" and "ordered" types.
     *
     *			--> TypeTable[] in ../main/util.c for  typeof()
     */

    /*  These exact numeric values are seldom used, but they are, e.g., in
     *  ../main/subassign.c
     */
    /*------ enum_SEXPTYPE ----- */
    typedef enum {
	NILSXP	= 0,	/* nil = NULL
			 *
			 * arr 2007/07/21: no RObject now has this
			 * type, but for backward compatibility TYPEOF
			 * will return NILSXP if passed a zero
			 * pointer.
			 */
	SYMSXP	= 1,	/* symbols */
	LISTSXP	= 2,	/* lists of dotted pairs */
	CLOSXP	= 3,	/* closures */
	ENVSXP	= 4,	/* environments */
	PROMSXP	= 5,	/* promises: [un]evaluated closure arguments */
	LANGSXP	= 6,	/* language constructs (special lists) */
	SPECIALSXP	= 7,	/* special forms */
	BUILTINSXP	= 8,	/* builtin non-special forms */
	CHARSXP	= 9,	/* "scalar" string type (internal only)*/
	LGLSXP	= 10,	/* logical vectors */
	INTSXP	= 13,	/* integer vectors */
	REALSXP	= 14,	/* real variables */
	CPLXSXP	= 15,	/* complex variables */
	STRSXP	= 16,	/* string vectors */
	DOTSXP	= 17,	/* dot-dot-dot object */
	ANYSXP	= 18,	/* make "any" args work */
	VECSXP	= 19,	/* generic vectors */
	EXPRSXP	= 20,	/* expressions vectors */
	BCODESXP    = 21,   /* byte code */
	EXTPTRSXP   = 22,   /* external pointer */
	WEAKREFSXP  = 23,   /* weak reference */
	RAWSXP      = 24,   /* raw bytes */
	S4SXP       = 25,   /* S4 non-vector */

	FUNSXP	= 99	/* Closure or Builtin */
    } SEXPTYPE;

#ifdef __cplusplus
}  // extern "C"

namespace CXXR {
    class PairList;
    class Symbol;

    /** @brief Replacement for CR's SEXPREC.
     *
     * This class is the rough equivalent within CXXR of the SEXPREC
     * union within CR.  However, all functionality relating to
     * garbage collection has been factored out into the base class
     * GCNode, and as CXXR development proceeds other functionality
     * will be factored out into derived classes (corresponding
     * roughly, but not exactly, to different SEXPTYPEs within CR).
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
     *
     * </ul>
     * The CR code in attrib.cpp applies further consistency
     * conditions on attributes, but these are not yet enforced via
     * the class interface.
     *
     * @todo Incorporate further attribute consistency checks within
     * the class interface.  Possibly make setAttribute() virtual so
     * that these consistency checks can be tailored according to the
     * derived class.
     *
     * @todo Possibly key attributes on (cached) strings rather than
     * Symbol objects.
     */
    struct RObject : public GCNode {
	/**
	 * @param stype Required type of the RObject.
	 */
	explicit RObject(SEXPTYPE stype = ANYSXP)
	    : m_type(stype), m_has_class(false), m_named(0), m_debug(false),
	      m_trace(false), m_attrib(0)
	{}

	/** @brief Get object attributes.
	 *
	 * @return Pointer to the attributes of this object.
	 *
	 * @deprecated This method allows clients to modify the
	 * attribute list directly, and thus bypass attribute
	 * consistency checks.
	 */
	PairList* attributes() {return m_attrib;}

	/** @brief Get object attributes (const variant).
	 *
	 * @return const pointer to the attributes of this object.
	 */
	const PairList* attributes() const {return m_attrib;}

	/** @brief Remove all attributes.
	 */
	void clearAttributes()
	{
	    m_attrib = 0;
	    m_has_class = false;
	}

	/** @brief Get the value a particular attribute.
	 *
	 * @param name Reference to a \c Symbol giving the name of the
	 *          sought attribute.  Note that this \c Symbol is
	 *          identified by its address.
	 *
	 * @return pointer to the value of the attribute with \a name,
	 * or a null pointer if there is no such attribute.
	 */
	RObject* getAttribute(const Symbol& name);

	/** @brief Get the value a particular attribute (const variant).
	 *
	 * @param name Reference to a \c Symbol giving the name of the
	 *          sought attribute.  Note that this \c Symbol is
	 *          identified by its address.
	 *
	 * @return const pointer to the value of the attribute with \a
	 * name, or a null pointer if there is no such attribute.
	 */
	const RObject* getAttribute(const Symbol& name) const;

	/** @brief Has this object any attributes?
	 *
	 * @return true iff this object has any attributes.
	 */
	bool hasAttributes() const
	{
	    return m_attrib != 0;
	}

	/** @brief Has this object the class attribute?
	 *
	 * @return true iff this object has the class attribute.
	 */
	bool hasClass() const
	{
	    return m_has_class;
	}

	/** @brief Set or remove an attribute.
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
	void setAttribute(Symbol* name, RObject* value);

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
	void setAttributes(PairList* new_attributes);

	// Virtual function of GCNode:
	void visitChildren(const_visitor* v) const;

	/** @brief Get an object's ::SEXPTYPE.
	 *
	 * @return ::SEXPTYPE of this object.
	 */
	SEXPTYPE sexptype() const {return m_type;}

	/** @brief Name within R of this type of object.
	 *
	 * @return the name by which this type of object is known
	 *         within R.
	 */
	virtual const char* typeName() const;

        // To be protected in future:

	virtual ~RObject() {}

    private:
	const SEXPTYPE m_type        : 7;
	bool m_has_class             : 1;
    public:
	// To be private in future:
	unsigned int m_named         : 2;
	bool m_debug                 : 1;
	bool m_trace                 : 1;
	FlagWord m_flags;
    private:
	PairList* m_attrib;
    };

    /* S4 object bit, set by R_do_new_object for all new() calls */
#define S4_OBJECT_MASK (1<<4)

}  // namespace CXXR

typedef CXXR::RObject SEXPREC, *SEXP;

extern "C" {
#else /* if not __cplusplus */

    typedef struct SEXPREC *SEXP;

#endif /* __cplusplus */

    /** @brief Get object's ::SEXPTYPE.
     *
     * @param x Pointer to CXXR::RObject.
     * @return ::SEXPTYPE of \a x, or NILSXP if x is a null pointer.
     */
#ifndef __cplusplus
    SEXPTYPE TYPEOF(SEXP x);
#else
    inline SEXPTYPE TYPEOF(SEXP x)  {return x ? x->sexptype() : NILSXP;}
#endif

    /** @brief Name of type within R.
     *
     * Translate a ::SEXPTYPE to the name by which it is known within R.
     * @param st The ::SEXPTYPE whose name is required.
     * @return The ::SEXPTYPE's name within R.
     */
    const char* Rf_type2char(SEXPTYPE st);

    /** @brief Copy attributes, with some exceptions.
     *
     * This is called in the case of binary operations to copy most
     * attributes from one of the input arguments to the output.
     * Note that the Dim, Dimnames and Names attributes are not
     * copied: these should have been assigned elsewhere.  The
     * function also copies the S4 object status.
     * @param inp Pointer to the CXXR::RObject from which attributes are to
     *          be copied.
     * @param ans Pointer to the CXXR::RObject to which attributes are to be
     *          copied.
     * @note The above documentation is probably incomplete: refer to the
     *       source code for further details.
     */
    void Rf_copyMostAttrib(SEXP inp, SEXP ans);

    /** @brief Access a named attribute.
     * @param vec Pointer to the CXXR::RObject whose attributes are to be
     *          accessed.
     * @param name Either a pointer to the symbol representing the
     *          required attribute, or a pointer to a CXXR::StringVector
     *          containing the required symbol name as element 0; in
     *          the latter case, as a side effect, the corresponding
     *          symbol is installed if necessary.
     * @return Pointer to the requested attribute, or a null pointer
     *         if there is no such attribute.
     * @note The above documentation is incomplete: refer to the
     *       source code for further details.
     */
    SEXP Rf_getAttrib(SEXP vec, SEXP name);

    /** @brief Set or remove a named attribute.
     * @param vec Pointer to the CXXR::RObject whose attributes are to be
     *          modified.
     * @param name Either a pointer to the symbol representing the
     *          required attribute, or a pointer to a CXXR::StringVector
     *          containing the required symbol name as element 0; in
     *          the latter case, as a side effect, the corresponding
     *          symbol is installed if necessary.
     * @param val Either the value to which the attribute is to be
     *          set, or a null pointer.  In the latter case the
     *          attribute (if present) is removed.
     * @return Refer to source code.  (Sometimes \a vec, sometimes \a
     * val, sometime a null pointer ...)
     * @note The above documentation is probably incomplete: refer to the
     *       source code for further details.
     */
    SEXP Rf_setAttrib(SEXP vec, SEXP name, SEXP val);

    /** @brief Does an object have a class attribute?
     *
     * @param x Pointer to a CXXR::RObject.
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

    /* Various tests */

    /**
     * @param s Pointer to a CXXR::RObject.
     * @return TRUE iff the CXXR::RObject pointed to by \a s is either a null
     * pointer (i.e. <tt>== R_NilValue</tt> in CXXR), or is a CXXR::RObject
     * with ::SEXPTYPE NILSXP (should not happen in CXXR).
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

    /** @brief Get the attributes of a CXXR::RObject.
     *
     * @param x Pointer to the CXXR::RObject whose attributes are required.
     * @return Pointer to the attributes object of \a x , or 0 if \a x is
     * a null pointer.
     */
    SEXP ATTRIB(SEXP x);

    /**
     * @deprecated
     */
#ifndef __cplusplus
    int LEVELS(SEXP x);
#else
    inline int LEVELS(SEXP x) {return x->m_flags.m_flags;}
#endif

    /** @brief Get object copying status.
     *
     * @param x Pointer to CXXR::RObject.
     * @return Refer to 'R Internals' document.  Returns 0 if \a x is a
     * null pointer.
     */
#ifndef __cplusplus
    int NAMED(SEXP x);
#else
    inline int NAMED(SEXP x) {return x ? x->m_named : 0;}
#endif

    /** @brief Get object tracing status.
     *
     * @param x Pointer to CXXR::RObject.
     * @return Refer to 'R Internals' document.  Returns 0 if \a x is a
     * null pointer.
     */
#ifndef __cplusplus
    int TRACE(SEXP x);
#else
    inline int TRACE(SEXP x) {return x ? x->m_trace : 0;}
#endif

    /**
     * @deprecated
     */
#ifndef __cplusplus
    int SETLEVELS(SEXP x, int v);
#else
    inline int SETLEVELS(SEXP x, int v) {return x->m_flags.m_flags = v;}
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
     * field of \x : refer to the documentation for \c
     * RObject::setAttributes() .  In particular, do not attempt to
     * modify the attributes by changing \a v \e after SET_ATTRIB
     * has been called.
     */
    void SET_ATTRIB(SEXP x, SEXP v);

    /** @brief Set object copying status.
     *
     * @param x Pointer to CXXR::RObject.  The function does nothing
     *          if \a x is a null pointer.
     * @param v Refer to 'R Internals' document.
     * @deprecated Ought to be private.
     */
#ifndef __cplusplus
    void SET_NAMED(SEXP x, int v);
#else
    inline void SET_NAMED(SEXP x, int v)
    {
	if (!x) return;
	x->m_named = v;
    }
#endif

#ifndef __cplusplus
    void SET_TRACE(SEXP x, int v);
#else
    inline void SET_TRACE(SEXP x, int v) {x->m_trace = v;}
#endif

    /** @brief Replace the attributes of \a to by those of \a from.
     *
     * @param to Pointer to CXXR::RObject.
     * @param from Pointer to another CXXR::RObject.
     */
    void DUPLICATE_ATTRIB(SEXP to, SEXP from);

    /* S4 object testing */

    /** @brief Is this an S4 object?
     *
     * @param x Pointer to CXXR::RObject.
     * @return true iff \a x is an S4 object.  Returns false if \a x
     * is 0.
     */
#ifndef __cplusplus
    Rboolean IS_S4_OBJECT(SEXP x);
#else
    inline Rboolean IS_S4_OBJECT(SEXP x)
    {
	return Rboolean(x && (x->m_flags.m_flags & S4_OBJECT_MASK));
    }
#endif

    /**
     * @deprecated Ought to be private.
     */
#ifndef __cplusplus
    void SET_S4_OBJECT(SEXP x);
#else
    inline void SET_S4_OBJECT(SEXP x)  {x->m_flags.m_flags |= S4_OBJECT_MASK;}
#endif

    /**
     * @deprecated Ought to be private.
     */
#ifndef __cplusplus
    void UNSET_S4_OBJECT(SEXP x);
#else
    inline void UNSET_S4_OBJECT(SEXP x)
    {
	x->m_flags.m_flags &= ~S4_OBJECT_MASK;
    }
#endif

    /** @brief Create an S4 object.
     *
     * @return Pointer to the created object.
     */
    SEXP Rf_allocS4Object();

    /* Bindings */
    /* use the same bits (15 and 14) in symbols and bindings */
#define ACTIVE_BINDING_MASK (1<<15)
#define BINDING_LOCK_MASK (1<<14)
#define SPECIAL_BINDING_MASK (ACTIVE_BINDING_MASK | BINDING_LOCK_MASK)

#ifndef __cplusplus
    Rboolean IS_ACTIVE_BINDING(SEXP b);
#else
    inline Rboolean IS_ACTIVE_BINDING(SEXP b)
    {
	return Rboolean(b->m_flags.m_flags & ACTIVE_BINDING_MASK);
    }
#endif

#ifndef __cplusplus
    Rboolean BINDING_IS_LOCKED(SEXP b);
#else
    inline Rboolean BINDING_IS_LOCKED(SEXP b)
    {
	return Rboolean(b->m_flags.m_flags & BINDING_LOCK_MASK);
    }
#endif

#ifndef __cplusplus
    void SET_ACTIVE_BINDING_BIT(SEXP b);
#else
    inline void SET_ACTIVE_BINDING_BIT(SEXP b)
    {
	b->m_flags.m_flags |= ACTIVE_BINDING_MASK;
    }
#endif

#ifndef __cplusplus
    void LOCK_BINDING(SEXP b);
#else
    inline void LOCK_BINDING(SEXP b) {b->m_flags.m_flags |= BINDING_LOCK_MASK;}
#endif

#ifndef __cplusplus
    void UNLOCK_BINDING(SEXP b);
#else
    inline void UNLOCK_BINDING(SEXP b)
    {
	b->m_flags.m_flags &= (~BINDING_LOCK_MASK);
    }
#endif

#ifdef __cplusplus
}
#endif

#endif /* ROBJECT_H */
