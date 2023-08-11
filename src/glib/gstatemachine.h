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
/// \file gstatemachine.h
///

#ifndef G_STATE_MACHINE_H
#define G_STATE_MACHINE_H

#include "gdef.h"
#include "gexception.h"
#include <map>

namespace G
{
	template <typename T, typename State, typename Event, typename Argument> class StateMachine ;
	class StateMachineImp ;
}

//| \class G::StateMachine
/// A finite state machine class template.
///
/// The finite state machine has a persistant 'state'. When an 'event' is
/// apply()d to the state machine, it undergoes a state 'transition'
/// and then calls the associated 'action' method.
///
/// Any action method can return a boolean predicate value which is used to
/// select between two transitions -- the 'normal' transition if the predicate
/// is true, and an 'alternative' transition if false.
///
/// Transition states can be implemented by having the relevant action
/// method call apply() on the state-machine. The state machine's state
/// is always changed before any action method is called -- although only
/// using the 'normal' transition, not the 'alternative' -- so this sort
/// of reentrancy is valid, as long as the action method going into
/// the transition state returns a predicate value of 'true'.
///
/// Default transitions for a given state are not supported directly. But
/// note that protocol errors do not invalidate the state machine and
/// do not result in a change of state. This means that client code can
/// achieve the effect of default transitions by handling protocol errors
/// for that state in a special manner.
///
/// Special states 'same' and 'any' can be defined to simplify the
/// definition of state transitions. A transition with a 'source'
/// state of 'any' will match any state. This is typically used
/// for error events or timeouts. A transition with a 'destination'
/// state of 'same' will not result in a state change. This is
/// sometimes used when handling predicates -- the predicate can
/// be used to control whether the state changes, or stays the
/// same. The 'any' state is also used as a return value from
/// apply() to signal a protocol error.
///
/// If the 'any' state is numerically the largest then it can be used
/// to identify a default transition for the given event; transitions
/// identified by an exact match with the current state will be
/// chosen in preference to the 'any' transition.
///
/// The 'end' state is special in that predicates are ignored for
/// transitions which have 'end' as their 'normal' destintation
/// state. This is because of a special implementation feature
/// which allows the state machine object to be deleted within the
/// action method which causes a transition to the 'end' state.
/// (This feature also means that transitions with an 'alternative'
/// state of 'end' are not valid.)
///
/// Usage:
/// \code
/// class Protocol
/// {
///   struct ProtocolError {} ;
///   enum class State { s_Same , sFoo , sBar , sEnd , s_Any } ;
///   enum class Event { eFoo , eBar , eError } ;
///   typedef StateMachine<Protocol,State,Event> Fsm ;
///   Fsm m_fsm ;
///   void doFoo( const std::string & , bool & ) {}
///   void doBar( const std::string & , bool & ) { delete this ; }
///   Event decode( const std::string & ) const ;
/// public:
///   Protocol() : m_fsm(State::sFoo,State::sBar,State::s_Same,State::s_Any)
///   {
///      m_fsm(Event::eFoo,State::sFoo,sBar,&Protocol::doFoo) ;
///      m_fsm(Event::eBar,State::sBar,sEnd,&Protocol::doBar) ;
///   }
///   void apply( const std::string & event_string )
///   {
///      State s = m_fsm.apply( *this , decode(event_string) , event_string ) ;
///      if( s == State::sEnd ) return ; // this already deleted by doBar()
///      if( s == State::sAny ) throw ProtocolError() ;
///   }
/// } ;
/// \endcode
///
template <typename T, typename State, typename Event, typename Argument>
class G::StateMachine
{
public:
	using Action = void (T::*)(Argument, bool &) ;

	StateMachine( State s_start , State s_end , State s_same , State s_any ) ;
		///< Constructor.

	void operator()( Event event , State from , State to , Action action ) ;
		///< Adds a transition. Special semantics apply if 'from' is
		///< 's_any', or if 'to' is 's_same'.

	void operator()( Event event , State from , State to , Action action , State alt ) ;
		///< An overload which adds a transition with predicate support.
		///< The 'alt' state is taken as an alternative 'to' state
		///< if the action's predicate is returned as false.

	State apply( T & t , Event event , Argument arg ) ;
		///< Applies an event. Calls the appropriate action method
		///< on object "t" and changes state. The state change
		///< takes into account the predicate returned by the
		///< action method.
		///<
		///< If the event is valid then the new state is returned.
		///< If the event results in a protocol error the StateMachine's
		///< state is unchanged, no action method is called, and
		///< this method returns 's_any' (see ctor).
		///<
		///< As a special implementation feature the StateMachine
		///< object may be deleted during the last action method
		///< callback (ie. the one which takes the state to the
		///< 's_end' state).

	State state() const ;
		///< Returns the current state.

	State reset( State new_state ) ;
		///< Sets the current state. Returns the old state.

	Event event() const ;
		///< Returns the last-apply()d event.

private:
	static void throwError() ;

private:
	struct Transition /// A private structure used by G::StateMachine<>.
	{
		State from ;
		State to ;
		State alt ; // alternate "to" state if predicate false
		Action action ;
		Transition(State s1,State s2,Action a,State s3) :
			from(s1) , to(s2) , alt(s3) , action(a) {}
	} ;
	using Map = std::multimap<Event,Transition> ;
	using Map_value_type = typename Map::value_type ;
	Map m_map ;
	State m_state ;
	State m_end ;
	State m_same ;
	State m_any ;
	Event m_event{} ;
} ;

//| \class G::StateMachine
/// A private non-template implementation class for G::StateMachine.
///
class G::StateMachineImp
{
public:
	G_EXCEPTION( Error , tx("invalid state transition") ) ;
	static void throwError() ;
	StateMachineImp() = delete ;
} ;

template <typename T, typename State, typename Event, typename Argument>
G::StateMachine<T,State,Event,Argument>::StateMachine( State s_start , State s_end , State s_same , State s_any ) :
	m_state(s_start) ,
	m_end(s_end) ,
	m_same(s_same) ,
	m_any(s_any)
{
}

template <typename T, typename State, typename Event, typename Argument>
void G::StateMachine<T,State,Event,Argument>::operator()( Event event , State from , State to , Action action )
{
	operator()( event , from , to , action , to ) ;
}

template <typename T, typename State, typename Event, typename Argument>
void G::StateMachine<T,State,Event,Argument>::operator()( Event event , State from , State to , Action action , State alt )
{
	if( to == m_any || alt == m_any || from == m_same ||
		( to == m_end && alt != to ) ||
		( alt == m_end && to != m_end ) )
			StateMachineImp::throwError() ;

	m_map.insert( Map_value_type( event , Transition(from,to,action,alt) ) ) ;
}

template <typename T, typename State, typename Event, typename Argument>
State G::StateMachine<T,State,Event,Argument>::reset( State new_state )
{
	State old_state = m_state ;
	m_state = new_state ;
	return old_state ;
}

template <typename T, typename State, typename Event, typename Argument>
State G::StateMachine<T,State,Event,Argument>::state() const
{
	return m_state ;
}

template <typename T, typename State, typename Event, typename Argument>
State G::StateMachine<T,State,Event,Argument>::apply( T & t , Event event , Argument arg )
{
	m_event = event ;
	State state = m_state ;
	auto p = m_map.find( event ) ; // look up in the multimap keyed on event + current-state
	for( ; p != m_map.end() && (*p).first == event ; ++p )
	{
		if( (*p).second.from == m_any || (*p).second.from == m_state )
		{
			State old_state = m_state ; // change state
			if( (*p).second.to != m_same )
				state = m_state = (*p).second.to ;

			State end = m_end ; // (avoid using members after the action method call)

			bool predicate = true ;
			(t.*((*p).second.action))( arg , predicate ) ; // perform action

			if( state != end && !predicate ) // respond to predicate
			{
				State alt_state = (*p).second.alt ;
				state = m_state = alt_state == m_same ? old_state : alt_state ;
			}
			return state ;
		}
	}
	return m_any ;
}

template <typename T, typename State, typename Event, typename Argument>
Event G::StateMachine<T,State,Event,Argument>::event() const
{
	return m_event ;
}

#endif

