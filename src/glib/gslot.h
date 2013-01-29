//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

/// \namespace G
namespace G
{

/// \class SlotBase
/// Part of the slot/signal system.
/// Used as a base class to all SlotImp<> classes
/// allowing them to be used as bodies to the
/// Slot<> reference-counting handle.
///
class SlotBase 
{
public:
	SlotBase() ;
		///< Default constuctor.

	virtual ~SlotBase() ;
		///< Destructor.

	void up() ;
		///< Increments the reference count.

	void down() ;
		///< Decrements the reference count
		///< and does "delete this" on zero.

private:
	SlotBase( const SlotBase & ) ; // not implemented
	void operator=( const SlotBase & ) ; // not implemented

private:
	unsigned long m_ref_count ;
} ;

/// \class SignalImp
/// Part of the slot/signal system.
/// A static helper class used by Signal<> classes.
///
class SignalImp 
{
public:
	G_EXCEPTION( AlreadyConnected , "signal already connected to a slot" ) ;
	static void check( const SlotBase * p ) ;
private:
	SignalImp() ; // not implemented
} ;

/// 
/// Part of the slot/signal system.
/// Compensates for having no std::swap in gcc2.95.
///
template <typename T>
void swap_( T & t1 , T & t2 )
{
	T temp( t1 ) ;
	t1 = t2 ;
	t2 = temp ;
}

///

/// \class SlotImp0
/// Part of the slot/signal system.
/// An implementation class for Slot0<>. An instance
/// is created by the slot<>()
///
template <typename T>
class SlotImp0 : public SlotBase 
{
private:
	T & m_object ;
	void (T::*m_fn)() ;
public:
	SlotImp0( T & object , void (T::*fn)() ) : m_object(object) , m_fn(fn) {}
	void callback() { (m_object.*m_fn)() ; }
} ;

/// \class SlotOp0
/// Part of the slot/signal system.
///
template <typename T>
class SlotOp0 
{
public:
	static void callback( SlotBase * imp )
		{ static_cast<SlotImp0<T>*>(imp)->callback() ; }
} ;

/// \class Slot0
/// Part of the slot/signal system.
///
class Slot0 
{
private:
	SlotBase * m_imp ;
	void (*m_op)( SlotBase * ) ;
public:
	Slot0() : m_imp(0) , m_op(0) {}
	Slot0( SlotBase * imp , void (*op)(SlotBase*) ) : m_imp(imp) , m_op(op) {}
	~Slot0() { if(m_imp) m_imp->down() ; }
	void callback() { if( m_imp ) (*m_op)( m_imp ) ; }
	Slot0( const Slot0 & other ) : m_imp(other.m_imp) , m_op(other.m_op) { if(m_imp) m_imp->up() ; }
	void swap( Slot0 & rhs ) { swap_(m_imp,rhs.m_imp) ; swap_(m_op,rhs.m_op) ; }
	void operator=( const Slot0 & rhs ) { Slot0 tmp(rhs) ; swap(tmp) ; }
	const SlotBase * base() const { return m_imp ; }
} ;

/// \class Signal0
/// Part of the slot/signal system.
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

/// 
/// Part of the slot/signal system.
///
template <typename T>
Slot0 slot( T & object , void (T::*fn)() )
{
	return Slot0( new SlotImp0<T>(object,fn) , SlotOp0<T>::callback ) ;
}

///

/// \class SlotImp1
/// Part of the slot/signal system.
///
template <typename T, typename P>
class SlotImp1 : public SlotBase 
{
private:
	T & m_object ;
	void (T::*m_fn)( P ) ;
public:
	SlotImp1( T & object , void (T::*fn)(P) ) : m_object(object) , m_fn(fn) {}
	void callback( P p ) { (m_object.*m_fn)(p) ; }
} ;

/// \class SlotOp1
/// Part of the slot/signal system.
///
template <typename T, typename P>
class SlotOp1 
{
public:
	static void callback( SlotBase * imp , P p )
		{ static_cast<SlotImp1<T,P>*>(imp)->callback( p ) ; }
} ;

/// \class Slot1
/// Part of the slot/signal system.
///
template <typename P>
class Slot1 
{
private:
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

/// \class Signal1
/// Part of the slot/signal system.
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

/// 
/// Part of the slot/signal system.
///
template <typename T,typename P>
Slot1<P> slot( T & object , void (T::*fn)(P) )
{
	return Slot1<P>( new SlotImp1<T,P>(object,fn) , SlotOp1<T,P>::callback ) ;
}

///

/// \class SlotImp2
/// Part of the slot/signal system.
///
template <typename T, typename P1, typename P2>
class SlotImp2 : public SlotBase 
{
private:
	T & m_object ;
	void (T::*m_fn)( P1 , P2 ) ;
public:
	SlotImp2( T & object , void (T::*fn)(P1,P2) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 ) { (m_object.*m_fn)(p1,p2) ; }
} ;

/// \class SlotOp2
/// Part of the slot/signal system.
///
template <typename T, typename P1 , typename P2>
class SlotOp2 
{
public:
	static void callback( SlotBase * imp , P1 p1 , P2 p2 )
		{ static_cast<SlotImp2<T,P1,P2>*>(imp)->callback( p1 , p2 ) ; }
} ;

/// \class Slot2
/// Part of the slot/signal system.
///
template <typename P1, typename P2>
class Slot2 
{
private:
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

/// \class Signal2
/// Part of the slot/signal system.
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

/// 
/// Part of the slot/signal system.
///
template <typename T, typename P1, typename P2>
Slot2<P1,P2> slot( T & object , void (T::*fn)(P1,P2) )
{
	return Slot2<P1,P2>( new SlotImp2<T,P1,P2>(object,fn) , SlotOp2<T,P1,P2>::callback ) ;
}

///

/// \class SlotImp3
/// Part of the slot/signal system.
///
template <typename T, typename P1, typename P2, typename P3>
class SlotImp3 : public SlotBase 
{
private:
	T & m_object ;
	void (T::*m_fn)( P1 , P2 , P3 ) ;
public:
	SlotImp3( T & object , void (T::*fn)(P1,P2,P3) ) : m_object(object) , m_fn(fn) {}
	void callback( P1 p1 , P2 p2 , P3 p3 ) { (m_object.*m_fn)(p1,p2,p3) ; }
} ;

/// \class SlotOp3
/// Part of the slot/signal system.
///
template <typename T, typename P1 , typename P2, typename P3>
class SlotOp3 
{
public:
	static void callback( SlotBase * imp , P1 p1 , P2 p2 , P3 p3 )
		{ static_cast<SlotImp3<T,P1,P2,P3>*>(imp)->callback( p1 , p2 , p3 ) ; }
} ;

/// \class Slot3
/// Part of the slot/signal system.
///
template <typename P1, typename P2, typename P3>
class Slot3 
{
private:
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

/// \class Signal3
/// Part of the slot/signal system.
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

/// 
/// Part of the slot/signal system.
///
template <typename T, typename P1, typename P2, typename P3>
Slot3<P1,P2,P3> slot( T & object , void (T::*fn)(P1,P2,P3) )
{
	return Slot3<P1,P2,P3>( new SlotImp3<T,P1,P2,P3>(object,fn) , SlotOp3<T,P1,P2,P3>::callback ) ;
}

}

#endif

