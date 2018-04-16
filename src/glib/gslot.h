//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gslot.h
///

#ifndef G_SLOT_H
#define G_SLOT_H

#include "gdef.h"
#include "gexception.h"
#include "gnoncopyable.h"

namespace G
{

/// \namespace G::Slot
///
/// A typesafe callback library that isolates event sinks from event sources.
///
/// The slot/signal pattern is used in several C++ libraries including
/// libsigc++, Qt and boost; it is completely unrelated to ANSI-C or POSIX
/// signals (signal(), sigaction(2)).
///
/// This implementation was inspired by libsigc++, but simplified by:
/// * not doing multicast
/// * not detecting dangling references
/// * not supporting global function callbacks
/// * using only void returns
///
/// Event-generating classes expose a "signal" object which client objects
/// can connect() to in order to receive events. The client receives events
/// through a "slot" member function.
///
/// The key to the implementation is that the slot implementation class
/// (eg. G::Slot::SlotImp1) is templated on the callback parameter P and
/// the callback sink class T, whereas the slot public interface (eg.
/// G::Slot::Slot1) is templated only on P. This means that the event
/// source, using the public interface, does not need to know the type of
/// the event sink. It is the slot callback classes (eg. G::Slot::Callback1)
/// that resolve the mismatch, by doing a down-cast to the T-specific
/// slot implementation class.
///
/// A slot is a reference-counting handle to a slot implementation body,
/// with the G::Slot::SlotImpBase class doing the reference counting.
/// Slots also hold a function pointer pointing to the relevant downcasting
/// method provided by one of the callback classes.
///
/// The overloaded factory function G::Slot::slot() creates a slot handle,
/// passing it a suitable SlotImpBase body and callback casting function.
/// The dynamic type of the SlotImpBase pointer is a slot implementation
/// class that knows about the specific sink class T, and the callback
/// casting function knows how to access the derived class. The combination
/// of SlotImpBase polymorphism and casting function isolates the slot
/// class from the sink class.
///
/// The signal classes contain a slot that is initially uninitialised;
/// the connect() method is used to initialise the slot so that it points
/// to the sink class's event handling function.
///
/// Usage:
/// \code
/// class Source
/// {
/// public:
///   G::Slot::Signal1<int> m_signal ;
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
///      source.m_signal.connect( G::Slot::slot(*this,&Sink::onEvent) ) ;
///   }
/// } ;
/// \endcode
///
namespace Slot
{

/// \class G::Slot::SlotImpBase
/// Used as a base class to all slot implementation classes (such as G::Slot::SlotImp1),
/// allowing them to be used as bodies to the associated slot reference-counting handle
/// class (eg. G::Slot::Slot1).
///
class SlotImpBase
{
public:
	SlotImpBase() ;
		///< Default constuctor.

	virtual ~SlotImpBase() ;
		///< Destructor.

	void up() ;
		///< Increments the reference count.

	void down() ;
		///< Decrements the reference count
		///< and does "delete this" on zero.

private:
	SlotImpBase( const SlotImpBase & ) ; // not implemented
	void operator=( const SlotImpBase & ) ; // not implemented

private:
	unsigned long m_ref_count ;
} ;

/// \class G::Slot::SignalImp
/// A static helper class used by G::Slot signal classes.
///
class SignalImp
{
public:
	G_EXCEPTION( AlreadyConnected , "signal already connected to a slot" ) ;
	static void check( const SlotImpBase * p ) ;
private:
	SignalImp() ; // not implemented
} ;

//

/// \class G::Slot::SlotImp0
/// A slot implementation class for zero-parameter callbacks.
///
template <typename T>
class SlotImp0 : public SlotImpBase
{
public:
	SlotImp0( T & object , void (T::*fn)() ) : m_object(object) , m_fn(fn) {}
	void callback() { (m_object.*m_fn)() ; }
private:
	T & m_object ;
	void (T::*m_fn)() ;
} ;

/// \class G::Slot::SlotCallback0
/// Provides a function to down-cast from SlotImpBase to SlotImp0.
///
template <typename T>
class SlotCallback0
{
public:
	static void callback( SlotImpBase * imp )
		{ static_cast<SlotImp0<T>*>(imp)->callback() ; }
} ;

/// \class G::Slot::Slot0
/// A slot class for zero-parameter callbacks.
///
class Slot0
{
private:
	SlotImpBase * m_imp ;
	void (*m_callback_fn)( SlotImpBase * ) ;
public:
	Slot0() : m_imp(0) , m_callback_fn(0) {}
	Slot0( SlotImpBase * imp , void (*op)(SlotImpBase*) ) : m_imp(imp) , m_callback_fn(op) {}
	~Slot0() { if(m_imp) m_imp->down() ; }
	void callback() { if( m_imp ) (*m_callback_fn)( m_imp ) ; }
	Slot0( const Slot0 & other ) : m_imp(other.m_imp) , m_callback_fn(other.m_callback_fn) { if(m_imp) m_imp->up() ; }
	void swap( Slot0 & rhs ) { using std::swap ; swap(m_imp,rhs.m_imp) ; swap(m_callback_fn,rhs.m_callback_fn) ; }
	void operator=( const Slot0 & rhs ) { Slot0 tmp(rhs) ; swap(tmp) ; }
	const SlotImpBase * base() const { return m_imp ; }
} ;

/// \class G::Slot::Signal0
/// A signal class for zero-parameter callbacks.
///
class Signal0 : public noncopyable
{
private:
	bool m_emitted ;
	bool m_once ;
	Slot0 m_slot ;
public:
	explicit Signal0( bool once = false ) : m_emitted(false) , m_once(once) {}
	void emit() { if(!m_once||!m_emitted) { m_emitted = true ; m_slot.callback() ; } }
	void connect( Slot0 slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot0() ; }
	void reset() { m_emitted = false ; }
} ;

/// A slot factory function overloaded for a zero-parameter callback.
template <typename T>
Slot0 slot( T & object , void (T::*fn)() )
{
	return Slot0( new SlotImp0<T>(object,fn) , SlotCallback0<T>::callback ) ;
}

//

/// \class G::Slot::SlotImp1
/// A slot implementation class for one-parameter callbacks.
///
template <typename T, typename P>
class SlotImp1 : public SlotImpBase
{
private:
	T & m_object ;
	void (T::*m_fn)( P ) ;
public:
	SlotImp1( T & object , void (T::*fn)(P) ) : m_object(object) , m_fn(fn) {}
	void callback( P p ) { (m_object.*m_fn)(p) ; }
} ;

/// \class G::Slot::SlotCallback1
/// Provides a function to down-cast from SlotImpBase to SlotImp1.
///
template <typename T, typename P>
class SlotCallback1
{
public:
	static void callback( SlotImpBase * imp , P p )
		{ static_cast<SlotImp1<T,P>*>(imp)->callback( p ) ; }
} ;

/// \class G::Slot::Slot1
/// A slot class for one-parameter callbacks.
///
template <typename P>
class Slot1
{
private:
	SlotImpBase * m_imp ;
	void (*m_callback_fn)( SlotImpBase * , P ) ;
public:
	Slot1() : m_imp(0) , m_callback_fn(0) {}
	Slot1( SlotImpBase * imp , void (*op)(SlotImpBase*,P) ) : m_imp(imp) , m_callback_fn(op) {}
	~Slot1() { if( m_imp ) m_imp->down() ; }
	void callback( P p ) { if( m_imp ) (*m_callback_fn)( m_imp , p ) ; }
	Slot1( const Slot1<P> & other ) : m_imp(other.m_imp) , m_callback_fn(other.m_callback_fn) { if(m_imp) m_imp->up() ; }
	void swap( Slot1<P> & rhs ) { using std::swap ; swap(m_imp,rhs.m_imp) ; swap(m_callback_fn,rhs.m_callback_fn) ; }
	void operator=( const Slot1<P> & rhs ) { Slot1 tmp(rhs) ; swap(tmp) ; }
	const SlotImpBase * base() const { return m_imp ; }
} ;

/// \class G::Slot::Signal1
/// A signal class for one-parameter callbacks.
///
template <typename P>
class Signal1 : public noncopyable
{
private:
	bool m_emitted ;
	bool m_once ;
	Slot1<P> m_slot ;
public:
	explicit Signal1( bool once = false ) : m_emitted(false) , m_once(once) {}
	void emit( P p ) { if(!m_once||!m_emitted) { m_emitted = true ; m_slot.callback( p ) ; } }
	void connect( Slot1<P> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot1<P>() ; }
	void reset() { m_emitted = false ; }
} ;

/// A slot factory function overloaded for a one-parameter callback.
template <typename T,typename P>
Slot1<P> slot( T & object , void (T::*fn)(P) )
{
	return Slot1<P>( new SlotImp1<T,P>(object,fn) , SlotCallback1<T,P>::callback ) ;
}

//

/// \class G::Slot::SlotImp2
/// A slot implementation class for two-parameter callbacks.
///
template <typename T, typename P1, typename P2>
class SlotImp2 : public SlotImpBase
{
private:
	T & m_object ;
	void (T::*m_fn)( P1 , P2 ) ;
public:
	SlotImp2( T & object , void (T::*fn)(P1,P2) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 ) { (m_object.*m_fn)(p1,p2) ; }
} ;

/// \class G::Slot::SlotCallback2
/// Provides a function to down-cast from SlotImpBase to SlotImp2.
///
template <typename T, typename P1 , typename P2>
class SlotCallback2
{
public:
	static void callback( SlotImpBase * imp , P1 p1 , P2 p2 )
		{ static_cast<SlotImp2<T,P1,P2>*>(imp)->callback( p1 , p2 ) ; }
} ;

/// \class G::Slot::Slot2
/// A slot class for two-parameter callbacks.
///
template <typename P1, typename P2>
class Slot2
{
private:
	SlotImpBase * m_imp ;
	void (*m_callback_fn)( SlotImpBase * , P1 , P2 ) ;
public:
	Slot2() : m_imp(0) , m_callback_fn(0) {}
	Slot2( SlotImpBase * imp , void (*op)(SlotImpBase*,P1,P2) ) : m_imp(imp) , m_callback_fn(op) {}
	~Slot2() { if( m_imp ) m_imp->down() ; }
	void callback( P1 p1 , P2 p2 ) { if( m_imp ) (*m_callback_fn)( m_imp , p1 , p2 ) ; }
	Slot2( const Slot2<P1,P2> & other ) : m_imp(other.m_imp) , m_callback_fn(other.m_callback_fn) { if(m_imp) m_imp->up() ; }
	void swap( Slot2<P1,P2> & rhs ) { using std::swap ; swap(m_imp,rhs.m_imp) ; swap(m_callback_fn,rhs.m_callback_fn) ; }
	void operator=( const Slot2<P1,P2> & rhs ) { Slot2 tmp(rhs) ; swap(tmp) ; }
	const SlotImpBase * base() const { return m_imp ; }
} ;

/// \class G::Slot::Signal2
/// A signal class for two-parameter callbacks.
///
template <typename P1, typename P2>
class Signal2 : public noncopyable
{
private:
	bool m_emitted ;
	bool m_once ;
	Slot2<P1,P2> m_slot ;
public:
	explicit Signal2( bool once = false ) : m_emitted(false) , m_once(once) {}
	void emit( P1 p1 , P2 p2 ) { if(!m_once||!m_emitted) { m_emitted = true ; m_slot.callback( p1 , p2 ) ; } }
	void connect( Slot2<P1,P2> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot2<P1,P2>() ; }
	void reset() { m_emitted = false ; }
} ;

/// A slot factory function overloaded for a two-parameter callback.
template <typename T, typename P1, typename P2>
Slot2<P1,P2> slot( T & object , void (T::*fn)(P1,P2) )
{
	return Slot2<P1,P2>( new SlotImp2<T,P1,P2>(object,fn) , SlotCallback2<T,P1,P2>::callback ) ;
}

//

/// \class G::Slot::SlotImp3
/// A slot implementation class for three-parameter callbacks.
///
template <typename T, typename P1, typename P2, typename P3>
class SlotImp3 : public SlotImpBase
{
private:
	T & m_object ;
	void (T::*m_fn)( P1 , P2 , P3 ) ;
public:
	SlotImp3( T & object , void (T::*fn)(P1,P2,P3) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 , P3 p3 ) { (m_object.*m_fn)(p1,p2,p3) ; }
} ;

/// \class G::Slot::SlotCallback3
/// Provides a function to down-cast from SlotImpBase to SlotImp3.
///
template <typename T, typename P1 , typename P2, typename P3>
class SlotCallback3
{
public:
	static void callback( SlotImpBase * imp , P1 p1 , P2 p2 , P3 p3 )
		{ static_cast<SlotImp3<T,P1,P2,P3>*>(imp)->callback( p1 , p2 , p3 ) ; }
} ;

/// \class G::Slot::Slot3
/// A slot class for three-parameter callbacks.
///
template <typename P1, typename P2, typename P3>
class Slot3
{
private:
	SlotImpBase * m_imp ;
	void (*m_callback_fn)( SlotImpBase * , P1 , P2 , P3 ) ;
public:
	Slot3() : m_imp(0) , m_callback_fn(0) {}
	Slot3( SlotImpBase * imp , void (*op)(SlotImpBase*,P1,P2,P3) ) : m_imp(imp) , m_callback_fn(op) {}
	~Slot3() { if( m_imp ) m_imp->down() ; }
	void callback( P1 p1 , P2 p2 , P3 p3 ) { if( m_imp ) (*m_callback_fn)( m_imp , p1 , p2 , p3 ) ; }
	Slot3( const Slot3<P1,P2,P3> & other ) : m_imp(other.m_imp) , m_callback_fn(other.m_callback_fn) { if(m_imp) m_imp->up() ; }
	void swap( Slot3<P1,P2,P3> & rhs ) { using std::swap ; swap(m_imp,rhs.m_imp) ; swap(m_callback_fn,rhs.m_callback_fn) ; }
	void operator=( const Slot3<P1,P2,P3> & rhs ) { Slot3 tmp(rhs) ; swap(tmp) ; }
	const SlotImpBase * base() const { return m_imp ; }
} ;

/// \class G::Slot::Signal3
/// A signal class for three-parameter callbacks.
///
template <typename P1, typename P2, typename P3>
class Signal3 : public noncopyable
{
private:
	bool m_emitted ;
	bool m_once ;
	Slot3<P1,P2,P3> m_slot ;
public:
	explicit Signal3( bool once = false ) : m_emitted(false) , m_once(once) {}
	void emit( P1 p1 , P2 p2 , P3 p3 ) { if(!m_once||!m_emitted) { m_emitted = true ; m_slot.callback( p1 , p2 , p3 ) ; }}
	void connect( Slot3<P1,P2,P3> slot ) { SignalImp::check(m_slot.base()) ; m_slot = slot ; }
	void disconnect() { m_slot = Slot3<P1,P2,P3>() ; }
	void reset() { m_emitted = false ; }
} ;

/// A slot factory function overloaded for a three-parameter callback.
template <typename T, typename P1, typename P2, typename P3>
Slot3<P1,P2,P3> slot( T & object , void (T::*fn)(P1,P2,P3) )
{
	return Slot3<P1,P2,P3>( new SlotImp3<T,P1,P2,P3>(object,fn) , SlotCallback3<T,P1,P2,P3>::callback ) ;
}

}
}

#endif
