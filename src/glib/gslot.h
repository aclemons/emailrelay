//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gslot.h
//
// Slots and signals provide a typesafe callback mechanism 
// that separates event source classes from event sinks.
// The slot/signal pattern is used in several C++ libraries
// including libsigc++, Qt and boost.
//
// This implementation was inspired by libsigc++,
// but simplified by:
// * not doing multicast
// * not detecting dangling references
// * not supporting global function callbacks
// * using only void returns
//
// Note that 'signals' in this context are not related 
// to ANSI-C or POSIX signals (signal(), sigaction(2)).
//
// Event-generating classes expose a "signal" object which
// client objects can connect() to in order to receive events. 
// The client receives events through a "slot" member function.
//
// The key to the implementation is that SlotImp<> is 
// templated on the callback parameter P and the callback 
// sink class T, but Slot<> is templated only on P, so 
// the event-source class does not need to know the type 
// of the event-sink. The SlotOp<> classes are used to 
// resolve this mismatch, by downcasting to the T-specific
// SlotImp<> class. The Slot<> classes are reference-counting 
// handles to SlotImp<> (via SlotBase), with an additional 
// function pointer for the relevant SlotOp<> downcasting 
// method.
//
// The overloaded template function slot<>() creates 
// a Slot<> handle, passing it a suitable SlotBase body 
// and casting function. The dynamic type of the SlotBase 
// reference is a SlotImp<> class which knows about the
// specific sink class T, and the casting function knows 
// how to access the derived class. The combination of
// SlotBase polymorphism and casting function isolates the
// slot class from the sink class.
//
// Usage:
/// class Source
/// {
/// public:
///   Signal1<int> m_signal ;
/// private:
///   void Source::raiseEvent()
///   {
///     int n = 123 ;
///     m_signal.emit( n ) ;
///   }
/// } ;
///
/// class Sink
/// {
/// public:
///   void onEvent( int n ) ;
///   Sink( Source & source ) 
///   { 
///      source.m_signal.connect( slot(*this,&Sink::onEvent) ) ; 
///   }
/// } ;
//

#ifndef G_SLOT_H
#define G_SLOT_H

#include "gdef.h"
#include "gexception.h"
#include "gnoncopyable.h"

namespace G
{

// Class: SlotBase
// Description: Part of the slot/signal system.
// Used as a base class to all SlotImp<> classes
// allowing them to be used as bodies to the
// Slot<> reference-counting handle.
//
class SlotBase 
{
public:
	virtual ~SlotBase() ;
		// Destructor.

	SlotBase() ;
		// Default constuctor.

	void up() ;
		// Increments the reference count.

	void down() ;
		// Decrements the reference count
		// and does "delete this" on zero.

private:
	SlotBase( const SlotBase & ) ; // not implemented
	void operator=( const SlotBase & ) ; // not implemented

private:
	unsigned long m_ref_count ;
} ;


// Class: SignalImp
// Description: Part of the slot/signal system.
// A static helper class used by Signal<> classes.
//
class SignalImp 
{
public:
	G_EXCEPTION( AlreadyConnected , "signal already connected to a slot" ) ;
	static void check( const SlotBase * p ) ;
private:
	SignalImp() ; // not implemented
} ;

template <class T>
inline
void swap_( T & t1 , T & t2 ) // no std::swap in gcc2.95
{
	T temp( t1 ) ;
	t1 = t2 ;
	t2 = temp ;
}

// ===

// Class: SlotImp0
// Description: Part of the slot/signal system.
// An implementation class for Slot0<>. An instance
// is created by the slot<>()
//
template <class T>
class SlotImp0 : public SlotBase 
{
	T & m_object ;
	void (T::*m_fn)() ;
public:
	SlotImp0( T & object , void (T::*fn)() ) : m_object(object) , m_fn(fn) {}
	void callback() { (m_object.*m_fn)() ; }
} ;

// Class: SlotOp0
// Description: Part of the slot/signal system.
//
template <class T>
class SlotOp0 
{
public:
	static void callback( SlotBase * imp )
		{ static_cast<SlotImp0<T>*>(imp)->callback() ; }
} ;

// Class: Slot0
// Description: Part of the slot/signal system.
//
class Slot0 
{
	SlotBase * m_imp ;
	void (*m_op)( SlotBase * ) ;
public:
	Slot0() : m_imp(0) , m_op(0) {}
	Slot0( SlotBase * imp , void (*op)(SlotBase*) ) : m_imp(imp) , m_op(op) {}
	~Slot0() { if( m_imp ) m_imp->down() ; }
	void callback() { if( m_imp ) (*m_op)( m_imp ) ; }
	Slot0( const Slot0 & other ) : m_imp(other.m_imp) , m_op(other.m_op) { if(m_imp) m_imp->up() ; }
	void swap( Slot0 & rhs ) { swap_(m_imp,rhs.m_imp) ; swap_(m_op,rhs.m_op) ; }
	void operator=( const Slot0 & rhs ) { Slot0 tmp(rhs) ; swap(tmp) ; }
	const SlotBase * base() const { return m_imp ; }
} ;

// Class: Signal0
// Description: Part of the slot/signal system.
//
class Signal0 : public noncopyable 
{
	Slot0 m_slot ;
public:
	Signal0() {}
	void emit() { m_slot.callback() ; }
	void connect( Slot0 slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot0() ; }
} ;

// Function: slot
// Description: Part of the slot/signal system.
//
template <class T>
inline
Slot0 slot( T & object , void (T::*fn)() )
{
	return Slot0( new SlotImp0<T>(object,fn) , SlotOp0<T>::callback ) ;
}

// ===

// Class: SlotImp1
// Description: Part of the slot/signal system.
//
template <class T, class P>
class SlotImp1 : public SlotBase 
{
	T & m_object ;
	void (T::*m_fn)( P ) ;
public:
	SlotImp1( T & object , void (T::*fn)(P) ) : m_object(object) , m_fn(fn) {}
	void callback( P p ) { (m_object.*m_fn)(p) ; }
} ;

// Class: SlotOp1
// Description: Part of the slot/signal system.
//
template <class T, class P>
class SlotOp1 
{
public:
	static void callback( SlotBase * imp , P p )
		{ static_cast<SlotImp1<T,P>*>(imp)->callback( p ) ; }
} ;

// Class: Slot1
// Description: Part of the slot/signal system.
//
template <class P>
class Slot1 
{
	SlotBase * m_imp ;
	void (*m_op)( SlotBase * , P ) ;
public:
	Slot1() : m_imp(0) , m_op(0) {}
	Slot1( SlotBase * imp , void (*op)(SlotBase*,P) ) : m_imp(imp) , m_op(op) {}
	~Slot1() { if( m_imp ) m_imp->down() ; }
	void callback( P p ) { if( m_imp ) (*m_op)( m_imp , p ) ; }
	Slot1( const Slot1<P> & other ) : m_imp(other.m_imp) , m_op(other.m_op) { if(m_imp) m_imp->up() ; }
	void swap( Slot1<P> & rhs ) { swap_(m_imp,rhs.m_imp) ; swap_(m_op,rhs.m_op) ; }
	void operator=( const Slot1<P> & rhs ) { Slot1 tmp(rhs) ; swap(tmp) ; }
	const SlotBase * base() const { return m_imp ; }
} ;

// Class: Signal1
// Description: Part of the slot/signal system.
//
template <class P>
class Signal1 : public noncopyable 
{
	Slot1<P> m_slot ;
public:
	Signal1() {}
	void emit( P p ) { m_slot.callback( p ) ; }
	void connect( Slot1<P> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot1<P>() ; }
} ;

// Function: slot
// Description: Part of the slot/signal system.
//
template <class T,class P>
inline
Slot1<P> slot( T & object , void (T::*fn)(P) )
{
	return Slot1<P>( new SlotImp1<T,P>(object,fn) , SlotOp1<T,P>::callback ) ;
}

// ===

// Class: SlotImp2
// Description: Part of the slot/signal system.
//
template <class T, class P1, class P2>
class SlotImp2 : public SlotBase 
{
	T & m_object ;
	void (T::*m_fn)( P1 , P2 ) ;
public:
	SlotImp2( T & object , void (T::*fn)(P1,P2) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 ) { (m_object.*m_fn)(p1,p2) ; }
} ;

// Class: SlotOp2
// Description: Part of the slot/signal system.
//
template <class T, class P1 , class P2>
class SlotOp2 
{
public:
	static void callback( SlotBase * imp , P1 p1 , P2 p2 )
		{ static_cast<SlotImp2<T,P1,P2>*>(imp)->callback( p1 , p2 ) ; }
} ;

// Class: Slot2
// Description: Part of the slot/signal system.
//
template <class P1, class P2>
class Slot2 
{
	SlotBase * m_imp ;
	void (*m_op)( SlotBase * , P1 , P2 ) ;
public:
	Slot2() : m_imp(0) , m_op(0) {}
	Slot2( SlotBase * imp , void (*op)(SlotBase*,P1,P2) ) : m_imp(imp) , m_op(op) {}
	~Slot2() { if( m_imp ) m_imp->down() ; }
	void callback( P1 p1 , P2 p2 ) { if( m_imp ) (*m_op)( m_imp , p1 , p2 ) ; }
	Slot2( const Slot2<P1,P2> & other ) : m_imp(other.m_imp) , m_op(other.m_op) { if(m_imp) m_imp->up() ; }
	void swap( Slot2<P1,P2> & rhs ) { swap_(m_imp,rhs.m_imp) ; swap_(m_op,rhs.m_op) ; }
	void operator=( const Slot2<P1,P2> & rhs ) { Slot2 tmp(rhs) ; swap(tmp) ; }
	const SlotBase * base() const { return m_imp ; }
} ;

// Class: Signal2
// Description: Part of the slot/signal system.
//
template <class P1, class P2>
class Signal2 : public noncopyable 
{
	Slot2<P1,P2> m_slot ;
public:
	Signal2() {}
	void emit( P1 p1 , P2 p2 ) { m_slot.callback( p1 , p2 ) ; }
	void connect( Slot2<P1,P2> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot2<P1,P2>() ; }
} ;

// Function: slot
// Description: Part of the slot/signal system.
//
template <class T, class P1, class P2>
inline
Slot2<P1,P2> slot( T & object , void (T::*fn)(P1,P2) )
{
	return Slot2<P1,P2>( new SlotImp2<T,P1,P2>(object,fn) , SlotOp2<T,P1,P2>::callback ) ;
}

// ===

// Class: SlotImp3
// Description: Part of the slot/signal system.
//
template <class T, class P1, class P2, class P3>
class SlotImp3 : public SlotBase 
{
	T & m_object ;
	void (T::*m_fn)( P1 , P2 , P3 ) ;
public:
	SlotImp3( T & object , void (T::*fn)(P1,P2,P3) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 , P3 p3 ) { (m_object.*m_fn)(p1,p2,p3) ; }
} ;

// Class: SlotOp3
// Description: Part of the slot/signal system.
//
template <class T, class P1 , class P2, class P3>
class SlotOp3 
{
public:
	static void callback( SlotBase * imp , P1 p1 , P2 p2 , P3 p3 )
		{ static_cast<SlotImp3<T,P1,P2,P3>*>(imp)->callback( p1 , p2 , p3 ) ; }
} ;

// Class: Slot3
// Description: Part of the slot/signal system.
//
template <class P1, class P2, class P3>
class Slot3 
{
	SlotBase * m_imp ;
	void (*m_op)( SlotBase * , P1 , P2 , P3 ) ;
public:
	Slot3() : m_imp(0) , m_op(0) {}
	Slot3( SlotBase * imp , void (*op)(SlotBase*,P1,P2,P3) ) : m_imp(imp) , m_op(op) {}
	~Slot3() { if( m_imp ) m_imp->down() ; }
	void callback( P1 p1 , P2 p2 , P3 p3 ) { if( m_imp ) (*m_op)( m_imp , p1 , p2 , p3 ) ; }
	Slot3( const Slot3<P1,P2,P3> & other ) : m_imp(other.m_imp) , m_op(other.m_op) { if(m_imp) m_imp->up() ; }
	void swap( Slot3<P1,P2,P3> & rhs ) { swap_(m_imp,rhs.m_imp) ; swap_(m_op,rhs.m_op) ; }
	void operator=( const Slot3<P1,P2,P3> & rhs ) { Slot3 tmp(rhs) ; swap(tmp) ; }
	const SlotBase * base() const { return m_imp ; }
} ;

// Class: Signal3
// Description: Part of the slot/signal system.
//
template <class P1, class P2, class P3>
class Signal3 : public noncopyable 
{
	Slot3<P1,P2,P3> m_slot ;
public:
	Signal3() {}
	void emit( P1 p1 , P2 p2 , P3 p3 ) { m_slot.callback( p1 , p2 , p3 ) ; }
	void connect( Slot3<P1,P2,P3> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot3<P1,P2,P3>() ; }
} ;

// Function: slot
// Description: Part of the slot/signal system.
//
template <class T, class P1, class P2, class P3>
inline
Slot3<P1,P2,P3> slot( T & object , void (T::*fn)(P1,P2,P3) )
{
	return Slot3<P1,P2,P3>( new SlotImp3<T,P1,P2,P3>(object,fn) , SlotOp3<T,P1,P2,P3>::callback ) ;
}

} // namespace

#endif

