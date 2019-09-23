//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
///
/// \file gcall.h
///

#ifndef G_CALL__H
#define G_CALL__H

#include "gdef.h"

namespace G
{
	class CallStack ;
	class CallFrame ;
}

/// \class G::CallStack
/// A linked list of CallFrame pointers.
///
/// The motivation is the situation where an object, typically
/// instantiated on the heap, emits some sort of synchronous
/// signal, event, or callback and the receiving code somehow
/// ends up deleting the originating object. If the emitting
/// object does more work before the stack unwinds then it can
/// protect itself with a CallFrame check, with almost zero
/// run-time cost:
///
/// \code
/// class Emitter
/// {
///   CallStack m_stack ;
///   void do_stuff()
///   {
///      CallFrame this_( m_stack ) ;
///      emit( "doing stuff" ) ; // call client code - can do anything
///      if( this_.deleted() ) return ; // just in case
///      do_more_stuff() ;
///   }
/// } ;
/// \endcode
///
class G::CallStack
{
public:
	CallStack() g__noexcept ;
		///< Constructor.

	~CallStack() g__noexcept ;
		///< Destructor. Calls invalidate() on all the frames in the stack.

	CallFrame * push( CallFrame * ) g__noexcept ;
		///< Pushes a new innermost call frame onto the stack.

	void pop( CallFrame * ) g__noexcept ;
		///< Makes the given frame the innermost.

private:
	CallStack( const CallStack & ) g__eq_delete ;
	void operator=( const CallStack & ) g__eq_delete ;

private:
	CallFrame * m_inner ;
} ;

/// \class G::CallFrame
/// An object to represent a nested execution context.
/// \see G::CallStack
///
class G::CallFrame
{
public:
	explicit CallFrame( CallStack & ) g__noexcept ;
		///< Constructor. The newly constructed call frame becomes
		///< the innermost frame in the stack.

	~CallFrame() g__noexcept ;
		///< Destructor.

	void invalidate() g__noexcept ;
		///< Invalidates the call-frame.

	bool valid() const g__noexcept ;
		///< Returns true if not invalidate()d. This is safe to call
		///< even if the call stack has been destructed.

	bool deleted() const g__noexcept ;
		///< Returns !valid().

	CallFrame * outer() g__noexcept ;
		///< Returns the next outermost frame in the stack.

private:
	CallFrame( const CallFrame & ) g__eq_delete ;
	void operator=( const CallFrame & ) g__eq_delete ;

private:
	CallStack & m_stack ;
	bool m_valid ;
	CallFrame * m_outer ;
} ;

// ==

inline
G::CallStack::CallStack() g__noexcept :
	m_inner(nullptr)
{
}

inline
G::CallStack::~CallStack() g__noexcept
{
	for( CallFrame * p = m_inner ; p ; p = p->outer() )
		p->invalidate() ;
}

inline
G::CallFrame * G::CallStack::push( CallFrame * p ) g__noexcept
{
	CallFrame * old = m_inner ;
	m_inner = p ;
	return old ;
}

inline
void G::CallStack::pop( CallFrame * p ) g__noexcept
{
	m_inner = p ;
}

// ==

inline
G::CallFrame::CallFrame( CallStack & stack ) g__noexcept :
	m_stack(stack) ,
	m_valid(true)
{
	m_outer = m_stack.push( this ) ;
}

inline
G::CallFrame::~CallFrame() g__noexcept
{
	if( m_valid )
		m_stack.pop( m_outer ) ;
}

inline
void G::CallFrame::invalidate() g__noexcept
{
	m_valid = false ;
}

inline
bool G::CallFrame::valid() const g__noexcept
{
	return m_valid ;
}

inline
bool G::CallFrame::deleted() const g__noexcept
{
	return !m_valid ;
}

inline
G::CallFrame * G::CallFrame::outer() g__noexcept
{
	return m_outer ;
}

#endif
