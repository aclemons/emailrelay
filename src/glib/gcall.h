//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_CALL_H
#define G_CALL_H

#include "gdef.h"

namespace G
{
	class CallStack ;
	class CallFrame ;
}

//| \class G::CallStack
/// A linked list of CallFrame pointers.
///
/// The motivation is the situation where an object, typically
/// instantiated on the heap, emits some sort of synchronous
/// signal, event, or callback and the receiving code somehow
/// ends up deleting the originating object. If the emitting
/// object might do more work before the stack unwinds then it
/// can protect itself with a CallFrame check, with almost zero
/// run-time cost:
///
/// \code
/// class Emitter
/// {
///   CallStack m_stack ;
///   void do_stuff()
///   {
///      CallFrame this_( m_stack ) ;
///      do_some_stuff() ;
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
	CallStack() noexcept ;
		///< Constructor.

	~CallStack() noexcept ;
		///< Destructor. Calls invalidate() on all the frames in the stack.

	CallFrame * push( CallFrame * ) noexcept ;
		///< Pushes a new innermost call frame onto the stack.

	void pop( CallFrame * ) noexcept ;
		///< Makes the given frame the innermost.

public:
	CallStack( const CallStack & ) = delete ;
	CallStack( CallStack && ) = delete ;
	CallStack & operator=( const CallStack & ) = delete ;
	CallStack & operator=( CallStack && ) = delete ;

private:
	CallFrame * m_inner{nullptr} ;
} ;

//| \class G::CallFrame
/// An object to represent a nested execution context.
/// \see G::CallStack
///
class G::CallFrame
{
public:
	explicit CallFrame( CallStack & ) noexcept ;
		///< Constructor. The newly constructed call frame becomes
		///< the innermost frame in the stack.

	~CallFrame() noexcept ;
		///< Destructor.

	void invalidate() noexcept ;
		///< Invalidates the call-frame.

	bool valid() const noexcept ;
		///< Returns true if not invalidate()d. This is safe to call
		///< even if the call stack has been destructed.

	bool deleted() const noexcept ;
		///< Returns !valid().

	CallFrame * outer() noexcept ;
		///< Returns the next frame in the stack going from innermost
		///< to outermost.

public:
	CallFrame( const CallFrame & ) = delete ;
	CallFrame( CallFrame && ) = delete ;
	CallFrame & operator=( const CallFrame & ) = delete ;
	CallFrame & operator=( CallFrame && ) = delete ;

private:
	CallStack & m_stack ;
	bool m_valid {true} ;
	CallFrame * m_outer ;
} ;

// ==

inline
G::CallStack::CallStack() noexcept
= default;

inline
G::CallStack::~CallStack() noexcept
{
	for( CallFrame * p = m_inner ; p ; p = p->outer() )
		p->invalidate() ;
}

inline
G::CallFrame * G::CallStack::push( CallFrame * p ) noexcept
{
	CallFrame * old = m_inner ;
	m_inner = p ;
	return old ;
}

inline
void G::CallStack::pop( CallFrame * p ) noexcept
{
	m_inner = p ;
}

// ==

inline
G::CallFrame::CallFrame( CallStack & stack ) noexcept :
	m_stack(stack) ,
	m_outer(stack.push(this))
{
}

inline
G::CallFrame::~CallFrame() noexcept
{
	if( m_valid )
		m_stack.pop( m_outer ) ;
}

inline
void G::CallFrame::invalidate() noexcept
{
	m_valid = false ;
}

inline
bool G::CallFrame::valid() const noexcept
{
	return m_valid ;
}

inline
bool G::CallFrame::deleted() const noexcept
{
	return !m_valid ;
}

inline
G::CallFrame * G::CallFrame::outer() noexcept
{
	return m_outer ;
}

#endif
