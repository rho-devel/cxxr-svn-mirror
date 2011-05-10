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
 *  Copyright (C) 1999-2006   The R Development Core Team.
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

/** @file Frame.hpp
 * @brief Class CXXR::Frame and associated functions.
 */

#ifndef RFRAME_HPP
#define RFRAME_HPP

#include "CXXR/GCNode.hpp"
#include "CXXR/PairList.h"
#include "CXXR/Symbol.h"

namespace CXXR {
    class Environment;
    class FunctionBase;

    /** @brief Mapping from Symbols to R objects.
     *
     * A Frame defines a mapping from (pointers to) CXXR::Symbol
     * objects to (pointers to) arbitrary objects of classes derived
     * from RObject.  A Frame is usually, but not necessarily,
     * associated with an Frame object.  Frame itself is an
     * abstract class; for most purposes its embodiment StdFrame
     * should be used.
     */
    class Frame : public GCNode {
    public:
	/** @brief Representation of a binding of a Symbol to an
	 *  RObject.
	 *
	 * A binding may be identified as 'active', in which case it
	 * encapsulates a function (i.e. an object of type
	 * FunctionBase).  The value of the binding, as returned by
	 * value(), is then determined by evaluating this function
	 * with no arguments.  Setting the value of the binding, by
	 * calling assign(), simply invokes that same function with
	 * the supplied value as its single argument: the encapsulated
	 * function is not altered.
	 *
	 * @note Arguably plain bindings and active bindings ought to
	 * be modelled by different classes.
	 */
	class Binding {
	public:
	    /** @brief How the binding arrived at its current setting.
	     */
	    enum Origin {
		EXPLICIT = 0, /**< The Binding was specified
			       * explicitly, e.g. by supplying an
			       * actual argument to a function, or by
			       * direct assignment into the relevant
			       * Environment.
			       */
		MISSING,      /**< The Binding corresponds to a formal
			       * argument of a function, for which no
			       * actual value was supplied, and which
			       * has no default value.  This is the
			       * default for newly created Binding
			       * objects.
			       */
		DEFAULTED     /**< The Binding represents the default
			       * value of the formal argument of a
			       * function call, no actual argument
			       * having been supplied.
			       */
	    };

	    /** @brief Default constructor
	     *
	     * initialize() must be called before the Binding object
	     * can be used.
	     */
	    Binding()
		: m_frame(0), m_symbol(0), m_value(Symbol::missingArgument()),
		  m_origin(MISSING), m_active(false), m_locked(false)
	    {}

	    /** @brief Represent this Binding as a PairList element.
	     *
	     * This function creates a new PairList element which
	     * represents the information in the Binding in the form
	     * returned by CR's FRAME() function.
	     *
	     * If the Binding is active, then the 'car' of the created
	     * PairList element will contain a pointer to the
	     * encapsulated function.
	     *
	     * @param tail Value to be set as the tail (CDR) of the
	     *          created PairList element.
	     *
	     * @return The created PairList element.
	     */
	    PairList* asPairList(PairList* tail = 0) const;

	    /** @brief Bind value to Symbol.
	     *
	     * In the case of non-active bindings, this function has
	     * exactly the same effect as setValue(): it changes the
	     * value to which this Binding's symbol is bound to \a
	     * new_value.
	     *
	     * For active bindings, it invokes the encapsulated
	     * function with \a new_value as its sole argument.
	     *
	     * Raises an error if the Binding is locked.
	     *
	     * @param new_value Pointer (possibly null) to the new
	     *          value.  See function description.
	     *
	     * @param origin Origin of the newly-assigned value.
	     */
	    void assign(RObject* new_value, Origin origin = EXPLICIT);

	    /** @brief Look up bound value, forcing Promises if
	     * necessary.
	     *
	     * If the value of this Binding is anything other than a
	     * Promise, this function returns a pointer to that bound
	     * value.  However, if the value is a Promise, the
	     * function forces the Promise if necessary, and returns a
	     * pointer to the value of the Promise.
	     *
	     * @return The first element of the returned pair is a
	     * pointer - possibly null - to the bound value, or the
	     * Promise value if the bound value is a Promise.  The
	     * second element is true iff the function call resulted
	     * in the forcing of a Promise.
	     *
	     * @note If this Binding's frame has a read monitor set,
	     * the function will call it only in the event that a
	     * Promise is forced (and in that event the evaluation of
	     * the Promise may trigger other read and write monitors).
	     *
	     * @note It is conceivable that forcing a Promise will
	     * result in the destruction of this Binding object.
	     */
	    std::pair<RObject*, bool> forcedValue();

	    /** @brief Get pointer to Frame.
	     *
	     * @return Pointer to the Frame to which this Binding
	     * belongs.
	     */
	    const Frame* frame() const
	    {
		return m_frame;
	    }

	    /** @brief Get binding information from a PairList
	     * element.
	     *
	     * This function sets the value of this Binding, and its
	     * active, locked and missing status, from the
	     * corresponding fields of a PairList element.
	     *
	     * If the PairList element defines the Binding as active,
	     * then its 'car' is considered to contain a pointer to
	     * the active binding function.
	     *
	     * Raises an error if the Binding is locked.
	     *
	     * @param pl Non-null pointer to the PairList element from
	     *          which binding information is to be obtained.
	     *          If \a pl has a tag, it must be a pointer to
	     *          the Symbol bound by this Binding.
	     */
	    void fromPairList(PairList* pl);

	    /** @brief Initialize the Binding.
	     *
	     * This function initializes the Frame and Symbol
	     * pointers of this Binding.  This function may be called
	     * at most once for any Binding object, and must be called
	     * before any other use is made of the Binding.
	     *
	     * @param frame Pointer to the Frame to which this
	     *          Binding belongs.  Must be non-null.
	     *
	     * @param sym Pointer to the Symbol bound by this
	     *          Binding.  Must be non-null.
	     */
	    void initialize(Frame* frame, const Symbol* sym);

	    /** @brief Is this an active Binding?
	     *
	     * @return true iff this is an active Binding.
	     */
	    bool isActive() const
	    {
		return m_active;
	    }

	    /** @brief Is this Binding locked?
	     *
	     * @return true iff this Binding is locked.
	     */
	    bool isLocked() const
	    {
		return m_locked;
	    }

	    /** @brief Origin of this Binding.
	     *
	     * @return the Origin of this Binding.
	     */
	    Origin origin() const
	    {
		return Origin(m_origin);
	    }

	    /** @brief Get raw value bound to the Symbol.
	     *
	     * 'raw' here means that in the case of an active Binding,
	     * the function returns a pointer to the encapsulated
	     * function rather than the result of evaluating the
	     * encapsulated function.
	     *
	     * @return The value bound to a Symbol by this Binding.
	     */
	    RObject* rawValue() const
	    {
		m_frame->monitorRead(*this);
		return m_value;
	    }

	    /** @brief Sets this to be an active Binding encapsulating
	     * a specified function.
	     *
	     * When invoked for an existing active Binding, this
	     * function simply replaces the encapsulated function.
	     *
	     * Raises an error if the Binding is locked.
	     *
	     * Also raises an error if the Binding is not currently
	     * marked active but has a non-null value.  (This is
	     * slightly less strict than CR, which only allows active
	     * status to be set on a newly created binding.)
	     *
	     * @param function The function used to implement the
	     *          active binding.
	     *
	     * @param origin Origin now to be associated with this Binding.
	     */
	    void setFunction(FunctionBase* function,
			     Origin origin = EXPLICIT);

	    /** @brief Lock/unlock this Binding.
	     *
	     * @param on true iff the Binding is to be locked.
	     */
	    void setLocking(bool on)
	    {
		m_locked = on;
	    }

	    /** @brief Define the object to which this Binding's
	     *         Symbol is bound.
	     *
	     * Raises an error if the Binding is locked or active.
	     *
	     * @param new_value Pointer (possibly null) to the RObject
	     *          to which this Binding's Symbol is now to be
	     *          bound.
	     *
	     * @param origin Origin of the newly assigned value.
	     */
	    void setValue(RObject* new_value, Origin origin = EXPLICIT);

	    /** @brief Bound symbol.
	     *
	     * @return Pointer to the Symbol bound by this Binding.
	     */
	    const Symbol* symbol() const
	    {
		return m_symbol;
	    }

	    /** @brief Get value bound to the Symbol.
	     *
	     * For an active binding, this evaluates the encapsulated
	     * function and returns the result rather than returning a
	     * pointer to the encapsulated function itself.
	     *
	     * @return The value bound to a Symbol by this Binding.
	     */
	    RObject* value() const;

	    /** @brief Auxiliary function to Frame::visitReferents().
	     *
	     * This function conducts a visitor to those objects
	     * derived from GCNode which become 'children' of this
	     * Binding's Frame as a result of its containing
	     * this Binding.
	     *
	     * @param v Pointer to the visitor object.
	     */
	    void visitReferents(const_visitor* v) const;
	private:
	    Frame* m_frame;
	    const Symbol* m_symbol;
	    GCEdge<> m_value;
	    unsigned char m_origin;
	    bool m_active;
	    bool m_locked;
	};

	typedef void (*monitor)(const Binding&);

	Frame()
	    : m_cache_count(0), m_locked(false),
	      m_read_monitored(false), m_write_monitored(false)
	{}

	/** @brief Copy constructor.
	 *
	 * The copy will define the same mapping from Symbols to R
	 * objects as \a source; neither the R objects, nor of course
	 * the Symbols, are copied as part of the cloning.
	 *
	 * The copy will be locked if \a source is locked.  However,
	 * the copy will not have a read or write monitor.
	 */
	Frame(const Frame& source)
	    : m_cache_count(0), m_locked(source.m_locked),
	      m_read_monitored(false), m_write_monitored(false)
	{}

	/** @brief Get contents as a PairList.
	 *
	 * Access the contents of this Frame expressed as a PairList,
	 * with the tag of each PairList element representing a Symbol
	 * and the car value representing the object to which that
	 * Symbol is mapped, and with the Binding's active, locked and
	 * missing status indicated as in CR's FRAME() function.
	 *
	 * @return pointer to a PairList as described above.
	 *
	 * @note The PairList is generated on demand, so this
	 * operation is relatively expensive in time.  Modifications
	 * to the returned PairList will have no effect on the
	 * Frame itself.
	 */
	virtual PairList* asPairList() const = 0;

	/** @brief Bind a Symbol to a specified value.
	 *
	 * @param symbol Non-null pointer to the Symbol to be bound or
	 *          rebound.
	 *
	 * @param value Pointer, possibly null, to the RObject to
	 *          which \a symbol is now to be bound.  Any previous
	 *          binding of \a symbol is overwritten.
	 *
	 * @param origin Origin of the newly bound value.
	 *
	 * @return Pointer to the resulting Binding.
	 */
	Binding* bind(const Symbol* symbol, RObject* value,
		      Frame::Binding::Origin origin = Frame::Binding::EXPLICIT)
	{
	    Binding* bdg = obtainBinding(symbol);
	    bdg->setValue(value, origin);
	    return bdg;
	}

	/** @brief Access binding of an already-defined Symbol.
	 *
	 * This function provides a pointer to the Binding of a
	 * Symbol.  In this variant the pointer is non-const, and
	 * consequently the calling code can use it to modify the
	 * Binding (provided the Binding is not locked).
	 *
	 * @param symbol The Symbol for which a mapping is sought.
	 *
	 * @return A pointer to the required binding, or a null
	 * pointer if it was not found.
	 */
#ifdef __GNUG__
	__attribute__((fastcall))
#endif
	virtual Binding* binding(const Symbol* symbol) = 0;

	/** @brief Access const binding of an already-defined Symbol.
	 *
	 * This function provides a pointer to a PairList element
	 * representing the binding of a symbol.  In this variant the
	 * pointer is const, and consequently the calling code can use
	 * it only to examine the binding.
	 *
	 * @param symbol The Symbol for which a mapping is sought.
	 *
	 * @return A pointer to the required binding, or a null
	 * pointer if it was not found..
	 */
	virtual const Binding* binding(const Symbol* symbol) const = 0;

	/** @brief Remove all symbols from the Frame.
	 *
	 * Raises an error if the Frame is locked.
	 */
	virtual void clear() = 0;

	/** @brief Return pointer to a copy of this Frame.
	 *
	 * This function creates a copy of this Frame, and returns a
	 * pointer to that copy.  The copy will define the same
	 * mapping from Symbols to R objects as this Frame; neither
	 * the R objects, nor of course the Symbols, are copied as
	 * part of the cloning.
	 *
	 * The created copy will be locked if this Frame is locked.
	 * However, it will not have a read or write monitor.
	 *
	 * @return a pointer to a clone of this Frame.
	 *
	 * @note Derived classes should exploit the covariant return
	 * type facility to return a pointer to the type of object
	 * being cloned.
	 */
	virtual Frame* clone() const = 0;

	/** @brief Remove the Binding (if any) of a Symbol.
	 *
	 * This function causes any Binding for a specified Symbol to
	 * be removed from the Frame.
	 *
	 * An error is raised if the Frame is locked (whether or not
	 * it contains a binding of \a symbol ).
	 *
	 * @param symbol The Symbol for which the Binding is to be
	 *          removed.
	 *
	 * @return true iff the environment previously contained a
	 * mapping for \a symbol.
	 */
	virtual bool erase(const Symbol* symbol) = 0;

	/** @brief Is the Frame locked?
	 *
	 * @return true iff the Frame is locked.
	 */
	bool isLocked() const
	{
	    return m_locked;
	}

	/** @brief Lock this Frame.
	 *
	 * Locking a Frame prevents the addition or removal of
	 * Bindings.  Optionally, the existing bindings can be locked,
	 * preventing them from being modified.
	 *
	 * This operation is permitted even if the Frame is already
	 * locked, but will have no effect unless it newly locks the
	 * Bindings in the Frame.
	 * 
	 * @param lock_bindings true iff all the existing Bindings in
	 *          the Frame are to be locked.
	 */
	void lock(bool lock_bindings)
	{
	    m_locked = true;
	    if (lock_bindings)
		lockBindings();
	}

	/** @brief Lock all Bindings in this Frame.
	 *
	 * This operation affects only Bindings currently existing.
	 * It does not prevent Bindings being added subsequently, and
	 * such Bindings will not be locked.
	 *
	 * It is permitted to apply this function to a locked Frame.
	 */
	virtual void lockBindings() = 0;

	/** @brief Number of Bindings in this Frame.
	 *
	 * @return The number of Bindings currently in this Frame.
	 */
	virtual std::size_t numBindings() const = 0;

	/** @brief Get or create a Binding for a Symbol.
	 *
	 * If the Frame already contains a Binding for a specified
	 * Symbol, the function returns it.  Otherwise a Binding to
	 * the null pointer is created, and a pointer to that Binding
	 * returned.
	 *
	 * An error is raised if a new Binding needs to be created and
	 * the Frame is locked.
	 *
	 * @param symbol The Symbol for which a Binding is to be
	 *          obtained.
	 *
	 * @return Pointer to the required Binding.
	 */
	virtual Binding* obtainBinding(const Symbol* symbol) = 0;

	/** @brief Monitor reading of Symbol values.
	 *
	 * This function allows the user to define a function to be
	 * called whenever a Symbol's value is read from a Binding
	 * within this Frame.
	 *
	 * In the case of an active Binding, the monitor is called
	 * whenever the encapsulated function is accessed: note that
	 * this includes calls to Binding::assign().
	 *
	 * @param new_monitor Pointer, possibly null, to the new
	 *          monitor function.  A null pointer signifies that
	 *          no read monitoring is to take place, which is the
	 *          default state.
	 *
	 * @return Pointer, possibly null, to the monitor being
	 * displaced by \a new_monitor.
	 *
	 * @note The presence or absence of a monitor is not
	 * considered to be part of the state of a Frame object, and
	 * hence this function is const.
	 */
	static monitor setReadMonitor(monitor new_monitor)
	{
	    monitor old = s_read_monitor;
	    s_read_monitor = new_monitor;
	    return old;
	}

	/** @brief Monitor writing of Symbol values.
	 *
	 * This function allows the user to define a function to be
	 * called whenever a Symbol's value is modified in a Binding
	 * within this Frame.
	 *
	 * In the case of an active Binding, the monitor is called
	 * only when the encapsulated function is initially set or
	 * changed: in particular the monitor is \e not invoked by
	 * calls to Binding::assign().
	 *
	 * The monitor is not called when a Binding is newly created
	 * within a Frame (with the Symbol bound by default to a null
	 * pointer).
	 *
	 * @param new_monitor Pointer, possibly null, to the new
	 *          monitor function.  A null pointer signifies that
	 *          no write monitoring is to take place, which is the
	 *          default state.
	 *
	 * @return Pointer, possibly null, to the monitor being
	 * displaced by \a new_monitor.
	 *
	 * @note The presence or absence of a monitor is not
	 * considered to be part of the state of a Frame object, and
	 * hence this function is const.
	 */
	static monitor setWriteMonitor(monitor new_monitor)
	{
	    monitor old = s_write_monitor;
	    s_write_monitor = new_monitor;
	    return old;
	}

	/** @brief Number of Symbols bound.
	 *
	 * @return the number of Symbols for which Bindings exist in
	 * this Frame.
	 */
	virtual std::size_t size() const = 0;

	/** @brief Merge this Frame's Bindings into another Frame.
	 *
	 * This function copies each Binding in this Frame into \a
	 * target, unless \a target already contains a Binding for the
	 * Symbol concerned.
	 *
	 * @param target Non-null pointer to the Frame into which
	 *          Bindings are to be merged.  An error is raised if
	 *          a new Binding needs to be created and \a target is
	 *          locked.
	 */
	virtual void softMergeInto(Frame* target) const = 0;

	/** @brief Symbols bound by this Frame.
	 *
	 * @param include_dotsymbols If false, any Symbol whose name
	 * begins with '.' will not be included in the result.
	 *
	 * @return A vector containing pointers to the Symbol objects
	 * bound by this Frame.
	 */
	virtual std::vector<const Symbol*>
	symbols(bool include_dotsymbols) const = 0;
    protected:
	// Declared protected to ensure that Frame objects are created
	// only using 'new':
	~Frame()
	{
	    statusChanged(0);
	}

	/** @brief Report change in the bound/unbound status of Symbol
	 *         objects.
	 *
	 * This function should be called when a Symbol that was not
	 * formerly bound within this Frame becomes bound, or <em>vice
	 * versa</em>.  If called with a null pointer, this signifies
	 * that all bindings are about to be removed from the Frame.
	 */
	void statusChanged(const Symbol* sym)
	{
	    if (m_cache_count > 0)
		flush(sym);
	}
    private:
	friend class Environment;

	static monitor s_read_monitor, s_write_monitor;

	unsigned char m_cache_count;  // Number of cached Environments
			// of which this is the Frame.  Normally
			// either 0 or 1.
	bool m_locked                  : 1;
	mutable bool m_read_monitored  : 1;
	mutable bool m_write_monitored : 1;

	// Not (yet) implemented.  Declared to prevent
	// compiler-generated versions:
	Frame& operator=(const Frame&);

	// Monitoring functions:
	friend class Binding;

	void decCacheCount()
	{
	    --m_cache_count;
	}

	// Flush symbol(s) from search list cache:
	void flush(const Symbol* sym);

	void incCacheCount()
	{
	    ++m_cache_count;
	}

	void monitorRead(const Binding& bdg) const
	{
	    if (m_read_monitored)
		s_read_monitor(bdg);
	}

	void monitorWrite(const Binding& bdg) const
	{
	    if (m_write_monitored)
		s_write_monitor(bdg);
	}
    };

    /** @brief Incorporate bindings defined by a PairList into a Frame.
     *
     * Raises an error if the Frame is locked, or an attempt is made
     * to modify a binding that is locked.
     *
     * @param env Pointer to the Frame into which new or
     *          modified bindings are to be incorporated.
     *
     * @param bindings List of symbol-value pairs defining bindings to
     *          be incorporated into the environment.  Every element
     *          of this list must have a Symbol as its tag (checked).
     *          If the list contains duplicate tags, later
     *          symbol-value pairs override earlier ones. Each
     *          resulting binding is locked and/or set active
     *          according to the m_active_binding and m_binding_locked
     *          fields of the corresponding PairList element.
     */
    void frameReadPairList(Frame* frame, PairList* bindings);

    /** @brief Does a Symbol correspond to a missing argument?
     *
     * Within a Frame \a frame, a Symbol \a sym is considered to
     * correspond to a missing argument if any of the following
     * criteria is satisfied:
     *
     * <ol>
     * <li>\a sym is itself Symbol::missingArgument()
     * (R_MissingArg).</li>
     *
     * <li>The binding of \a sym within \a frame is flagged as having
     * origin Frame::Binding::MISSING.</li>
     *
     * <li>\a sym is bound to Symbol::missingArgument().</li>
     *
     * <li>\a sym is bound to a unforced Promise, and forcing the
     * Promise would consist in evaluating a Symbol which - by a
     * recursive application of these criteria - is missing with
     * respect to the Frame of the Environment of the Promise.</li>
     *
     * <ol>
     *
     * Note that unless Criterion 1 applies, \a sym is not considered
     * missing if it is not bound at all within \a frame, or if it has
     * an active binding.
     *
     * @param sym Non-null pointer to the Symbol whose missing status
     *          is to be determined.
     *
     * @param frame Non-null pointer to the Frame with respect to
     *          which missingness is to be determined.
     *
     * @return true iff \a sym is missing with respect to \a frame.
     */
    bool isMissingArgument(const Symbol* sym, Frame* frame);
}  // namespace CXXR

// This definition is visible only in C++; C code sees instead a
// definition (in Environment.h) as an opaque pointer.
typedef CXXR::Frame::Binding* R_varloc_t;

#endif // RFRAME_HPP
