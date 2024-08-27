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
/// \file geventstate.cpp
///

#include "gdef.h"
#include "gnetdone.h"
#include "geventstate.h"
#include "geventloop.h"
#include "glog.h"
#include "gassert.h"
#include <exception>

namespace GNet
{
	namespace EventStateImp
	{
		struct LogExceptionHandler : ExceptionHandler /// A GNet::ExceptionHandler that just logs.
		{
			void onException( ExceptionSource * , std::exception & e , bool net_done ) override
			{
				if( !net_done )
					G_LOG( "GNet::EventState: exception: " << e.what() ) ;
			}
		} ;
	}
}

GNet::EventState::EventState( Private , ExceptionHandler * eh , ExceptionSource * esrc ) noexcept :
	m_eh(eh) ,
	m_esrc(esrc)
{
}

GNet::EventState GNet::EventState::create( std::nothrow_t )
{
	static EventStateImp::LogExceptionHandler log_only_exception_handler ;
	EventState es( Private() , nullptr , nullptr ) ;
	es.m_eh = &log_only_exception_handler ;
	return es ;
}

GNet::EventState GNet::EventState::create()
{
	return { Private() , nullptr , nullptr } ;
}

GNet::EventState GNet::EventState::logging( EventLogging * logging ) const noexcept
{
	// note that in normal usage the logging pointer will be valid but its ctor may not have run
	G_ASSERT( logging != nullptr ) ;
	EventState copy( *this ) ;
	copy.m_logging = logging ;
	return copy ;
}

GNet::EventState GNet::EventState::esrc( Private , ExceptionSource * esrc ) const noexcept
{
	EventState es( *this ) ;
	es.m_esrc = esrc ;
	return es ;
}

void GNet::EventState::doOnException( std::exception & e , bool done )
{
	G_ASSERT( m_eh != nullptr ) ; // precondition -- see EventEmitter and TimerList
	m_eh->onException( m_esrc , e , done ) ;
}

void GNet::EventState::disarm() noexcept
{
	m_eh = nullptr ;
	m_esrc = nullptr ;
}

// ==

GNet::EventStateUnbound::EventStateUnbound( EventState es ) noexcept :
	m_es(es)
{
	m_es.esrc( EventState::Private() , nullptr ) ;
}

GNet::EventState GNet::EventStateUnbound::bind( ExceptionSource * esrc ) const noexcept
{
	return m_es.esrc( EventState::Private() , esrc ) ;
}

