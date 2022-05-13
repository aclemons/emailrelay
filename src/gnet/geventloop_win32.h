//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geventloop_win32.h
///

#ifndef G_NET_EVENTLOOP_WIN32_H
#define G_NET_EVENTLOOP_WIN32_H

#include "gdef.h"
#include "gevent.h"
#include "gassert.h"
#include "glog.h"
#include <memory>
#include <new>
#include <vector>
#include <algorithm>

namespace GNet
{
	class EventLoopImp ;
	class EventLoopHandles ;
}

//| \class GNet::EventLoopImp
/// A Windows event loop.
///
/// The implementation use GNet::EventLoopHandles to do the low-level
/// WaitForMultipleObjects().
///
class GNet::EventLoopImp : public EventLoop
{
public:
	EventLoopImp() ;
		///< Default constructor.

	virtual ~EventLoopImp() ;
		///< Destructor.

private: // overrides
	std::string run() override ;
	bool running() const override ;
	void quit( const std::string & ) override ;
	void quit( const G::SignalSafe & ) override ;
	void disarm( ExceptionHandler * ) noexcept override ;
	void addRead( Descriptor , EventHandler & , ExceptionSink ) override ;
	void addWrite( Descriptor , EventHandler & , ExceptionSink ) override ;
	void addOther( Descriptor , EventHandler & , ExceptionSink ) override ;
	void dropRead( Descriptor ) noexcept override ;
	void dropWrite( Descriptor ) noexcept override ;
	void dropOther( Descriptor ) noexcept override ;

public:
	EventLoopImp( const EventLoopImp & ) = delete ;
	EventLoopImp( EventLoopImp && ) = delete ;
	void operator=( const EventLoopImp & ) = delete ;
	void operator=( EventLoopImp && ) = delete ;

private:
	static constexpr long READ_EVENTS = (FD_READ | FD_ACCEPT | FD_OOB) ;
	static constexpr long WRITE_EVENTS = (FD_WRITE) ; // no need for "FD_CONNECT"
	static constexpr long EXCEPTION_EVENTS = (FD_CLOSE) ;
	using s_type = G::TimeInterval::s_type ;
	using us_type = G::TimeInterval::us_type ;
	struct Library
	{
		Library() ;
		~Library() ;
		Library( const Library & ) = delete ;
		Library( Library && ) = delete ;
		void operator=( const Library & ) = delete ;
		void operator=( Library && ) = delete ;
	} ;

public:
	enum class RcType // A type enumeration for GNet::EventLoopImp::Rc.
	{
		timeout ,
		event ,
		message ,
		failed ,
		other
	}  ;
	struct Rc /// A structure for the return code from WaitForMultiplObjects().
	{
		inline Rc( RcType type , std::size_t index = 0U , std::size_t imp_1 = 0U , std::size_t imp_2 = 0U ) noexcept :
			m_type(type) ,
			m_index(index) ,
			m_imp_1(imp_1) ,
			m_imp_2(imp_2)
		{
		}
		inline RcType type() const noexcept { return m_type ; }
		inline operator RcType () const noexcept { return m_type ; }
		inline std::size_t index() const noexcept { return m_index ; }
		RcType m_type ;
		std::size_t m_index ; // ListItem index
		std::size_t m_imp_1 ; // optional value used by EventLoopHandles implementation
		std::size_t m_imp_2 ; // optional value used by EventLoopHandles implementation
	} ;
	enum class ListItemType // A type enumeration for the list of event sources.
	{
		socket ,
		simple ,
		other
	} ;
	struct ListItem /// A structure holding an event handle and its event handlers.
	{
		explicit ListItem( Descriptor fdd ) :
			m_type(fdd.fd()==INVALID_SOCKET?ListItemType::simple:ListItemType::socket) ,
			m_socket(fdd.fd()) ,
			m_handle(fdd.h())
		{
		}
		explicit ListItem( HANDLE h ) :
			m_type(ListItemType::other) ,
			m_handle(h) ,
			m_events(~0L)
		{
		}
		Descriptor fd() const
		{
			return Descriptor( m_socket , m_handle ) ;
		}
		ListItemType m_type{ListItemType::socket} ;
		SOCKET m_socket{INVALID_SOCKET} ;
		HANDLE m_handle{HNULL} ;
		long m_events{0L} ; // 0 => new or garbage
		EventEmitter m_read_emitter ;
		EventEmitter m_write_emitter ;
		EventEmitter m_other_emitter ;
	} ;
	using List = std::vector<ListItem> ;

private:
	ListItem * find( Descriptor ) ;
	ListItem & findOrCreate( Descriptor ) ;
	void updateSocket( Descriptor , long ) ;
	bool updateSocket( Descriptor , long , std::nothrow_t ) noexcept ;
	void handleSimpleEvent( ListItem & ) ;
	void handleSocketEvent( std::size_t ) ;
	void checkForOverflow( const ListItem & ) ;
	DWORD ms() ;
	static DWORD ms( s_type , us_type ) noexcept ;
	static bool isValid( const ListItem & ) ;
	static bool isInvalid( const ListItem & ) ;

private:
	Library m_library ;
	List m_list ;
	std::unique_ptr<EventLoopHandles> m_handles ;
	bool m_running ;
	bool m_dirty ;
	bool m_quit ;
} ;

#endif
