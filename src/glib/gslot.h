//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gassert.h"
#include <functional>
#include <memory>

#ifdef emit
// beware Qt
#error invalid preprocessor definition of 'emit'
#endif

namespace G
{
	//| \namespace G::Slot
	/// A callback mechanism that isolates event sinks from event sources.
	///
	/// The slot/signal pattern has been used in several C++ libraries including
	/// libsigc++, Qt and boost, although it is largely redudant with modern C++.
	/// The pattern is completely unrelated to ANSI-C or POSIX signals (signal(),
	/// sigaction(2)).
	///
	/// Usage:
	/// \code
	/// struct Source
	/// {
	///   G::Slot::Signal<int> m_signal ;
	///   void Source::raiseEvent()
	///   {
	///     m_signal.emit( 123 ) ;
	///   }
	/// } ;
	///
	/// struct Sink
	/// {
	///   void onEvent( int n ) ;
	///   Sink( Source & source ) : m_source(source)
	///   {
	///     source.m_signal.connect( G::Slot::slot(*this,&Sink::onEvent) ) ;
	///   }
	///   ~Sink()
	///   {
	///     m_source.m_signal.disconnect() ;
	///   }
	///   Sink( const Sink & ) = delete ;
	///   Sink( Sink && ) noexcept { rebind() ; }
	///   Sink & operator=( const Sink & ) = delete ;
	///   Sink & operator=( Sink && ) noexcept { rebind() ; return *this ; }
	///   void rebind() noexcept( check( m_source.m_signal.rebind(*this) ) ; }
	///   Source & m_source ;
	/// } ;
	/// \endcode
	///
	/// For comparison the equivalent modern C++ looks like this:
	/// \code
	/// struct Source
	/// {
	///   std::function<void(int)> m_signal ;
	///   void Source::raiseEvent()
	///   {
	///     if( m_signal ) m_signal( 123 ) ;
	///   }
	/// } ;
	///
	/// struct Sink
	/// {
	///   void onEvent( int n ) ;
	///   Sink( Source & source ) : m_source(source)
	///   {
	///     throw_already_connected_if( !source.m_signal ) ;
	///     source.m_signal = std::bind_front(&Sink::onEvent,this) ;
	///   }
	///   ~Sink()
	///   {
	///     m_source.m_signal = nullptr ;
	///   }
	///   Source & m_source ;
	/// } ;
	/// \endcode
	///
	/// Slot methods should take parameters by value or const reference but
	/// beware of emit()ing references to data members of objects that might
	/// get deleted. Use temporaries in the emit() call if in doubt.
	///
	namespace Slot
	{
		//| \class G::Slot::Binder
		/// A functor class template that contains the target object pointer
		/// and method pointer, similar to c++20 bind_front(&T::fn,tp).
		/// These objects are hidden in the std::function data member of
		/// the Slot class so that the Slot is not dependent on the target
		/// type. Maybe replace with a lambda.
		///
		template <typename T, typename... Args>
		struct Binder
		{
			using Mf = void (T::*)(Args...) ;
			T * m_sink ;
			Mf m_mf ;
			Binder( T * sink , Mf mf ) :
				m_sink(sink) ,
				m_mf(mf)
			{
			}
			void rebind( T * sink ) noexcept
			{
				m_sink = sink ;
			}
			void operator()( Args... args )
			{
				return (m_sink->*m_mf)( args... ) ;
			}
		} ;

		//| \class G::Slot::Slot
		/// A slot class template that is parameterised only on the target method's
		/// signature (with an implicit void return) and not on the target class.
		/// The implementation uses std::function to hide the type of the target.
		///
		template <typename... Args>
		struct Slot
		{
			std::function<void(Args...)> m_fn ;
			Slot() noexcept = default;
			template <typename T> Slot( T & sink , void (T::*mf)(Args...) ) :
				m_fn(std::function<void(Args...)>(Binder<T,Args...>(&sink,mf)))
			{
			}
			explicit Slot( std::function<void(Args...)> fn ) :
				m_fn(fn)
			{
			}
			void invoke( Args... args )
			{
				if( m_fn )
					m_fn( args... ) ;
			}
			template <typename T> bool rebind( T & sink ) noexcept
			{
				using BinderType = Binder<T,Args...> ;
				bool rebindable = m_fn && m_fn.template target<BinderType>() ;
				if( rebindable )
					m_fn.template target<BinderType>()->rebind( &sink ) ;
				return rebindable ;
			}
		} ;

		//| \class G::Slot::SignalImp
		/// A slot/signal scoping class.
		///
		struct SignalImp
		{
			G_EXCEPTION_CLASS( AlreadyConnected , tx("signal already connected") )
			SignalImp() = delete ;
		} ;

		//| \class G::Slot::Signal
		/// A slot holder, with connect() and emit() methods.
		///
		template <typename... SlotArgs>
		struct Signal
		{
			Slot<SlotArgs...> m_slot ;
			bool m_once ;
			bool m_emitted {false} ;
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
				m_slot.m_fn = nullptr ;
				G_ASSERT( !connected() ) ;
			}
			void emit( SlotArgs... args )
			{
				if( !m_once || !m_emitted )
				{
					m_emitted = true ;
					if( connected() )
						m_slot.invoke( args... ) ;
				}
			}
			void reset() noexcept
			{
				m_emitted = false ;
			}
			bool connected() const
			{
				return !! m_slot.m_fn ;
			}
			bool emitted() const noexcept
			{
				return m_emitted ;
			}
			void emitted( bool emitted ) noexcept
			{
				m_emitted = emitted ;
			}
			template <typename T> bool rebind( T & sink ) noexcept
			{
				return m_slot.rebind( sink ) ;
			}
			~Signal() = default ;
			Signal( const Signal & ) = delete ;
			Signal( Signal && ) noexcept = default ;
			Signal & operator=( const Signal & ) = delete ;
			Signal & operator=( Signal && ) noexcept = default ;
		} ;

		/// A factory function for Slot objects.
		///
		template <typename TSink,typename... Args> Slot<Args...> slot( TSink & sink , void (TSink::*method)(Args...) )
		{
			// or c++20: return std::function<void(Args...)>( std::bind_front(method,&sink) )
			return Slot<Args...>( sink , method ) ;
		}
	}
}
#endif
