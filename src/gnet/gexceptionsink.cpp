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
/// \file gexceptionsink.cpp
///

#include "gdef.h"
#include "gnetdone.h"
#include "gexceptionsink.h"
#include "geventloop.h"
#include "glog.h"
#include "gassert.h"
#include <exception>

namespace GNet
{
	namespace ExceptionSinkImp
	{
		struct LogExceptionHandler : ExceptionHandler /// A GNet::ExceptionHandler that just logs.
		{
			void onException( ExceptionSource * , std::exception & e , bool net_done ) override
			{
				if( !net_done )
					G_LOG( "GNet::ExceptionSink: exception: " << e.what() ) ;
			}
		} ;
	}
}

GNet::ExceptionSink::ExceptionSink() noexcept
= default ;

GNet::ExceptionSink::ExceptionSink( ExceptionHandler & eh , ExceptionSource * esrc ) noexcept :
	m_eh(&eh) ,
	m_esrc(esrc)
{
}

GNet::ExceptionSink::ExceptionSink( ExceptionHandler * eh , ExceptionSource * esrc ) noexcept :
	m_eh(eh) ,
	m_esrc(esrc)
{
}

GNet::ExceptionSink GNet::ExceptionSink::logOnly()
{
	static ExceptionSinkImp::LogExceptionHandler log_only_exception_handler ;
	return { log_only_exception_handler , nullptr } ;
}

#ifndef G_LIB_SMALL
GNet::ExceptionSink GNet::ExceptionSink::rethrow()
{
	return {} ;
}
#endif

GNet::ExceptionHandler * GNet::ExceptionSink::eh() const noexcept
{
	return m_eh ;
}

GNet::ExceptionSource * GNet::ExceptionSink::esrc() const noexcept
{
	return m_esrc ;
}

void GNet::ExceptionSink::call( std::exception & e , bool done )
{
	G_ASSERT( m_eh != nullptr ) ; // precondition -- see EventEmitter and TimerList
	m_eh->onException( m_esrc , e , done ) ;
}

void GNet::ExceptionSink::reset() noexcept
{
	m_eh = nullptr ;
	m_esrc = nullptr ;
}

bool GNet::ExceptionSink::set() const noexcept
{
	return m_eh != nullptr ;
}

// ==

#ifndef G_LIB_SMALL
GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler & eh ) :
	m_eh(&eh)
{
}
#endif

GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler * eh ) :
	m_eh(eh)
{
	G_ASSERT( eh != nullptr ) ;
}

GNet::ExceptionSink GNet::ExceptionSinkUnbound::bind( ExceptionSource * source ) const
{
	return { m_eh , source } ;
}

