//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_SLOT_NEW_H
#define G_SLOT_NEW_H

#include "gdef.h"
#include "gexception.h"
#include "gassert.h"
#include <functional>
#include <memory>

namespace G
{
	/// \namespace G::Slot
	/// A callback mechanism that isolates event sinks from event sources.
	///
	/// The slot/signal pattern is used in several C++ libraries including
	/// libsigc++, Qt and boost; it is completely unrelated to ANSI-C or POSIX
	/// signals (signal(), sigaction(2)).
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
	/// Slots can also be used on their own, without being connected to
	/// signals, just like std::bind and std::function:
	///
	/// \code
	/// struct Generator ;
	/// struct Test
	/// {
	///   void run()
	///   {
	///     // cf. std::bind(&Test::value,this,_1)
	///     Generator generator( G::Slot::slot(*this,&Test::value) ) ;
	///     generator.generateValues() ;
	///   }
	///   void value( int n ) ;
	/// } ;
	/// struct Generator
	/// {
	///   Generator( G::Slot::Slot1<int> callback ) :
	///     m_callback(callback)
	///   {
	///   }
	///   void generateValues()
	///   {
	///     for( int i = 0 ; i < 10 ; i++ )
	///       m_callback.callback( i ) ;
	///   }
	///   // cf. std::function<void(int)>
	///   G::Slot::Slot1<int> m_callback ;
	/// } ;
	/// \endcode
	///
	/// The implementation of emit() uses perfect forwarding so beware
	/// of emit()ing references to data members of objects that might
	/// get deleted. Use temporaries in the emit() call if in doubt.
	///
	namespace Slot
	{
		/// \class G::Slot::SlotImp
		/// A sink-specific functor class template. The types of these functors
		/// are hidden by the std::function in the Slot class.
		///
		template <typename Treturn, typename Tsink, typename... Args>
		struct SlotImp
		{
			using Mf = Treturn (Tsink::*)(Args...) ;
			Tsink * m_sink ;
			Mf m_mf ;
			SlotImp( Tsink * sink , Mf mf ) :
				m_sink(sink) ,
				m_mf(mf)
			{
			}
			Treturn operator()( Args&&... args )
			{
				return (m_sink->*m_mf)( std::forward<Args>(args)... ) ;
			}
		} ;

		/// \class G::Slot::Slot
		/// A slot class template that is parameterised only on the target method's
		/// signature (with an implicit void return) and not on the target class.
		///
		template <typename... Args>
		struct Slot
		{
			using Treturn = void ;
			using std_function_type = std::function<Treturn (Args...)> ;
			std_function_type m_fn ;
			Slot() noexcept = default;
			void reset() noexcept { m_fn = nullptr ; }
			template <typename Tsink> Slot( Tsink & sink , Treturn (Tsink::*mf)(Args...) ) :
				m_fn(std::function<Treturn(Args...)>(SlotImp<Treturn,Tsink,Args...>(&sink,mf)))
			{
			}
			void callback( Args... args ) // (for backwards compatibility)
			{
				m_fn( std::forward<Args>(args)... ) ;
			}
		} ;

		/// \class G::Slot::SignalImp
		/// A slot/signal scoping class.
		///
		struct SignalImp
		{
			G_EXCEPTION_CLASS( AlreadyConnected , "already connected" ) ;
			SignalImp() = delete ;
		} ;

		/// \class G::Slot::Signal
		/// A slot holder, with connect() and emit() methods.
		///
		template <typename... SlotArgs>
		struct Signal
		{
			Slot<SlotArgs...> m_slot ;
			bool m_once ;
			bool m_emitted{false} ;
			explicit Signal( bool once = false ) :
				m_once(once)
			{
			}
			void connect( Slot<SlotArgs...> slot )
			{
				if( m_slot.m_fn ) throw SignalImp::AlreadyConnected() ;
				m_slot = slot ;
			}
			void disconnect() noexcept
			{
				m_slot.reset() ;
				G_ASSERT( !connected() ) ;
			}
			template <typename... SignalArgs>
			void emit( SignalArgs&&... args )
			{
				if( !m_once || !m_emitted )
				{
					m_emitted = true ;
					if( connected() )
						m_slot.m_fn( std::forward<SignalArgs>(args)... ) ;
				}
			}
			void reset()
			{
				m_emitted = false ;
			}
			bool connected() const
			{
				return !! m_slot.m_fn ;
			}
			~Signal() = default ;
			Signal( const Signal & ) = delete ;
			Signal( Signal && ) = delete ;
			void operator=( const Signal & ) = delete ;
			void operator=( Signal && ) = delete ;
		} ;

		/// A factory function for Slot objects.
		///
		template <typename TSink,typename... Args> Slot<Args...> slot( TSink & sink , void (TSink::*method)(Args...) )
		{
			return Slot<Args...>( sink , method ) ;
		}

		// backwards compatible names...
		using Signal0 = Signal<> ;
		template <typename T> using Signal1 = Signal<T> ;
		template <typename T1, typename T2> using Signal2 = Signal<T1,T2> ;
		template <typename T1, typename T2, typename T3> using Signal3 = Signal<T1,T2,T3> ;
		template <typename T1, typename T2, typename T3, typename T4> using Signal4 = Signal<T1,T2,T3,T4> ;
		template <typename T1, typename T2, typename T3, typename T4, typename T5> using Signal5 = Signal<T1,T2,T3,T4,T5> ;
		using Slot0 = Slot<> ;
		template <typename T> using Slot1 = Slot<T> ;
		template <typename T1, typename T2> using Slot2 = Slot<T1,T2> ;
		template <typename T1, typename T2, typename T3> using Slot3 = Slot<T1,T2,T3> ;
		template <typename T1, typename T2, typename T3, typename T4> using Slot4 = Slot<T1,T2,T3,T4> ;
		template <typename T1, typename T2, typename T3, typename T4, typename T5> using Slot5 = Slot<T1,T2,T3,T4,T5> ;
	}
}
#endif
