//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gexceptionsink.cpp
//

#include "gdef.h"
#include "gnetdone.h"
#include "gexceptionsink.h"
#include "geventloop.h"
#include "gassert.h"

namespace
{
	struct RethrowExceptionHandler : public GNet::ExceptionHandler
	{
		virtual void onException( GNet::ExceptionSource * , std::exception & , bool ) override
		{
			throw ;
		}
	} rethrow_exception_handler ;
	struct LogExceptionHandler : public GNet::ExceptionHandler
	{
		virtual void onException( GNet::ExceptionSource * , std::exception & e , bool done ) override
		{
			if( !done )
				G_ERROR( "GNet::ExceptionSink: exception: " << e.what() ) ;
		}
	} log_exception_handler ;
}

GNet::ExceptionSink::ExceptionSink( Type type , ExceptionSource * /*esrc*/ ) :
	m_eh(type==Type::Null?nullptr:&rethrow_exception_handler) ,
	m_esrc(nullptr)
{
}

GNet::ExceptionSink::ExceptionSink( ExceptionHandler & eh , ExceptionSource * esrc ) :
	m_eh(&eh) ,
	m_esrc(esrc)
{
}

GNet::ExceptionSink::ExceptionSink( ExceptionHandler * eh , ExceptionSource * esrc ) :
	m_eh(eh) ,
	m_esrc(esrc)
{
	G_ASSERT( eh != nullptr ) ;
}

GNet::ExceptionHandler * GNet::ExceptionSink::eh() const
{
	return m_eh ;
}

GNet::ExceptionSource * GNet::ExceptionSink::esrc() const
{
	return m_esrc ;
}

void GNet::ExceptionSink::call( std::exception & e , bool done )
{
	if( m_eh != nullptr )
	{
		m_eh->onException( m_esrc , e , done ) ;
	}
}

void GNet::ExceptionSink::reset()
{
	m_eh = nullptr ;
	m_esrc = nullptr ;
}

bool GNet::ExceptionSink::set() const
{
	return m_eh != nullptr ;
}

// ==

GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler & eh ) :
	m_eh(&eh)
{
}

GNet::ExceptionSinkUnbound::ExceptionSinkUnbound( ExceptionHandler * eh ) :
	m_eh(eh)
{
	G_ASSERT( eh != nullptr ) ;
}

GNet::ExceptionSink GNet::ExceptionSinkUnbound::bind( ExceptionSource * source ) const
{
	return ExceptionSink( m_eh , source ) ;
}

/// \file gexceptionsink.cpp
